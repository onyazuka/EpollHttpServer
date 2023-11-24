#include "TcpServer.hpp"
#include <arpa/inet.h>
#include "ProjLogger.hpp"
#include <string.h>

using namespace inet::tcp;

AddrInfo::AddrInfo(std::string_view ipv4, uint16_t port) 
	: _sAddr{ipv4.data(), ipv4.size()}, _port{port}
{
	if (inet_pton(AF_INET, ipv4.data(), &(_addr.sin_addr)) < 0) {
		throw InvalidAddrException();
	}
	_addr.sin_family = AF_INET;
	_addr.sin_port = htons(port);
}

TcpServer::Options::Options(bool _nonBlock) 
	: nonBlock{_nonBlock}
{
	;
}

TcpServer::TcpServer(std::string_view ipv4, uint16_t port, Options&& _opts)
	: addrInfo(ipv4, port), opts{ std::move(_opts) }, socketMapper{}, threadPool{}
{
	if (init() < 0) {
		throw std::runtime_error("Server init error");
	}
	if (run() < 0) {
		throw std::runtime_error("Server run error");
	}
}

int TcpServer::init() {
	for (size_t i = 0; i < threadPool.size(); ++i) {
		threadPool.getThreadObj(i).setMapper(&socketMapper);
	}
	return 0;
}

int TcpServer::run() {
	Log.debug("Server creating socket");
	serverFd = socket(AF_INET, SOCK_STREAM | (opts.nonBlock ? SOCK_NONBLOCK : 0), 0);
	if (serverFd < 0) {
		Log.error(std::format("Error while creating socket: {}", strerror(errno)));
		return -1;
	}

	Log.debug(std::format("Server binding socket {} on ip {} and port {}", serverFd, addrInfo.sAddr(), addrInfo.port()));
	const auto& addr = addrInfo.sockAddr();
	if (bind(serverFd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		Log.error(std::format("Error while binding socket {}: {}", serverFd, strerror(errno)));
		serverClose();
		return -1;
	}

	Log.debug(std::format("Server listening socket {}", serverFd));
	if (listen(serverFd, MAX_LISTENING_CLIENTS) < 0) {
		Log.error(std::format("Error while listening socket {}: {}", serverFd, strerror(errno)));
		serverClose();
		return -1;
	}

	Log.debug("Server creating epoll");
	epollFd = epoll_create1(0);
	if (epollFd < 0) {
		Log.error(std::format("Error while creating epoll: {}", strerror(errno)));
		serverClose();
		return -1;
	}

	Log.debug("Server epoll_ctl");
	struct epoll_event event, events[MAX_EPOLL_EVENTS];
	event.events = EPOLLIN | EPOLLET /*| EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR */;
	event.data.fd = serverFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) < 0) {
		Log.error(std::format("Error on epoll ctl", strerror(errno)));
		serverClose();
		return -1;
	}
	
	//uint32_t ss = 0;
	//uint32_t len = sizeof(ss);
	//int err = getsockopt(serverFd, SOL_SOCKET, SO_RCVBUF, (char*)&ss, &len);

	while (true) {
		int numEvents = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, -1);
		if (numEvents < 0) {
			Log.error(std::format("Error on epoll {} waiting:", epollFd, strerror(errno)));
			serverClose();
			return -1;
		}
		//Log.debug(std::format("Recv {} events", numEvents));
		for (int i = 0; i < numEvents; ++i) {
			if (events[i].data.fd == serverFd) {
				// handle new connection
				struct sockaddr_in clientAddr;
				socklen_t clientAddrLen = sizeof(clientAddr);
				int clientFd = accept(serverFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
				if ((clientFd < 0) && (errno != EAGAIN)) {
					Log.error(std::format("Failed to accept client connection: {}", strerror(errno)));
					continue;
				}
				else if (errno == EAGAIN) {
					;
				}
				else {
					event.events = EPOLLIN | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
					event.data.fd = clientFd;
					if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientFd, &event) < 0) {
						Log.error(std::format("Failed to add client socket to epoll instance", strerror(errno)));
						close(clientFd);
						continue;
					}
					Log.debug(std::format("Handling client {}", clientFd));
				}
			}
			else {
				int clientFd = events[i].data.fd;
				size_t threadIdx = 0;
				if (auto optThreadIdx = socketMapper.findThreadIdx(clientFd); optThreadIdx.has_value()) {
					threadIdx = optThreadIdx.value();
					//Log.debug(std::format("Got existing idx {} for fd {}", threadIdx, clientFd));
				}
				else {
					threadIdx = threadPool.getIdx();
					socketMapper.addThreadIdx(clientFd, threadIdx);
					//Log.debug(std::format("Got new idx {} for fd {}", threadIdx, clientFd));
				}
				auto& threadCtx = threadPool.getThreadObj(threadIdx);
				if (events[i].events & EPOLLIN) {
					threadPool.pushTask(threadIdx, std::function([&threadCtx](int epollFd, int clientFd) { threadCtx.onInputData(epollFd, clientFd); return 0; }), std::move(epollFd), std::move(clientFd));
					//threadCtx.onInputData(epollFd, clientFd);
				}
				else if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
					threadPool.pushTask(threadIdx, std::function([&threadCtx](int epollFd, int clientFd) { threadCtx.onError(epollFd, clientFd); return 0; }), std::move(epollFd), std::move(clientFd));
					//threadCtx.onError(epollFd, clientFd);

					Log.error(std::format("Client connection closed {}", clientFd));
					epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
					close(clientFd);
					continue;
				}
				continue;
			}
		}
		// cooldown sleep to reduce number of small events
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}



	return 0;
}

void TcpServer::serverClose() {
	Log.debug(std::format("Closing server socket {}", serverFd));
	if (serverFd >= 0) close(serverFd);
	if (epollFd >= 0) close(epollFd);
}