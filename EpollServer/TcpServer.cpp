#include "TcpServer.hpp"
#include <arpa/inet.h>
#include <format>
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
	: addrInfo(ipv4, port), opts{std::move(_opts)}
{
	if (run() < 0) {
		throw std::runtime_error("Server error");
	}
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
	event.events = EPOLLIN /* | EPOLLOUT | EPOLLHUP | EPOLLRDHUP | EPOLLERR */;
	event.data.fd = serverFd;
	if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverFd, &event) < 0) {
		Log.error(std::format("Error on epoll ctl", strerror(errno)));
		serverClose();
		return -1;
	}

	while (true) {
		int numEvents = epoll_wait(epollFd, events, MAX_EPOLL_EVENTS, -1);
		if (numEvents < 0) {
			Log.error(std::format("Error on epoll {} waiting:", epollFd, strerror(errno)));
			serverClose();
			return -1;
		}
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
				if (events[i].events & EPOLLIN) {
					Log.debug(std::format("Handling client data {}", clientFd));
					bool err = false;
					for (;;) {
						ssize_t n = read(clientFd, buf, sizeof(buf));
						if (n <= 0 && (errno != EAGAIN)) {
							epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
							close(clientFd);
							err = true;
							break;
						}
						else if (errno == EAGAIN)
						{
							continue;
						}
						else {
							Log.debug(std::format("Read {} bytes from {}", n, clientFd));
							//break;
						}
					}
					if (err) {
						;
					}
				}
				else if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
					Log.error(std::format("Client connection closed {}", clientFd));
					epoll_ctl(epollFd, EPOLL_CTL_DEL, clientFd, NULL);
					close(clientFd);
					continue;
				}
				continue;
			}
		}
	}



	return 0;
}

void TcpServer::serverClose() {
	Log.debug(std::format("Closing server socket {}", serverFd));
	if (serverFd >= 0) close(serverFd);
	if (epollFd >= 0) close(epollFd);
}