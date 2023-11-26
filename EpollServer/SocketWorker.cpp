#include "SocketWorker.hpp"
#include "ProjLogger.hpp"
#include <sys/epoll.h>
#include <string.h>

using namespace inet;

SocketDataHandler::SocketDataHandler(QueueT&& tasksQueue)
	: tasksQueue{ std::move(tasksQueue) }
{
	;
}

SocketDataHandler::SocketDataHandler(SocketDataHandler&& other) noexcept {
	std::lock_guard<std::mutex> lck(other.mtx);
	tasksQueue = std::move(other.tasksQueue);
	thread = std::move(other.thread);
	sockBufs = std::move(other.sockBufs);
	mapper = other.mapper;
}

SocketDataHandler& SocketDataHandler::operator=(SocketDataHandler&& other) noexcept {
	std::lock_guard<std::mutex> lck(other.mtx);
	tasksQueue = std::move(other.tasksQueue);
	thread = std::move(other.thread);
	sockBufs = std::move(other.sockBufs);
	mapper = other.mapper;
	return *this;
}

SocketDataHandler::~SocketDataHandler() {
	request_stop();
	join();
}

void SocketDataHandler::request_stop() {
	thread.request_stop();
}

void SocketDataHandler::join() {
	if (thread.joinable()) {
		thread.join();
	}
}

SocketDataHandler::QueueT& SocketDataHandler::queue() {
	return tasksQueue;
}

void SocketDataHandler::setMapper(SocketThreadMapper* _mapper) {
	assert(_mapper);
	mapper = _mapper;
}

void SocketDataHandler::onInputData(int epollFd, std::shared_ptr<ISocket> clientSock) {
	int fd = clientSock->fd();
	if (!checkFd(clientSock)) return;
	//cout << format("Reading from epoll {} and socket {}\n", epollFd, socketFd);
	auto& buf = sockBufs[fd];
	if (buf.empty()) {
		// new connection
		// NOT reserve
		buf.resize(BUF_SIZE);
	}
	Log.debug(std::format("Handling client data {}", fd));
	if (ssize_t nbytes = clientSock->read(buf); nbytes <= 0 && nbytes != -EAGAIN) {
		onError(epollFd, clientSock);
	}
}

void SocketDataHandler::onError(int epollFd, std::shared_ptr<ISocket> clientSock) {
	Log.error(std::format("Closing connection from server with client {}", clientSock->fd()));
	onCloseClient(epollFd, clientSock);
}

void SocketDataHandler::onCloseClient(int epollFd, std::shared_ptr<ISocket> clientSock) {
	int fd = clientSock->fd();
	if (!checkFd(clientSock)) return;
	epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	mapper->removeFd(clientSock->fd());
	sockBufs.erase(fd);
}

bool SocketDataHandler::checkFd(std::shared_ptr<ISocket> sock) {
	// fd may have been already closed
	if (auto [_sock, threadIdx] = mapper->findThreadIdx(sock->fd()); _sock == nullptr) {
		return false;
	}
	return true;
}

void SocketDataHandler::run() {
	thread = std::move(std::jthread([this](std::stop_token stop) {
		while (!stop.stop_requested()) {
			TaskT task;
			if (tasksQueue.popWaitFor(task, std::chrono::milliseconds(1000))) {
				task.first(task.second);
			}
		}
		}));
}

std::pair<SocketThreadMapper::SockT, size_t> SocketThreadMapper::findThreadIdx(int fd) {
	std::shared_lock<std::shared_mutex> lck(mtx);
	if (auto iter = map.find(fd); iter == map.end()) {
		return { nullptr, 0 };
	}
	else {
		return iter->second;
	}
}
void SocketThreadMapper::addThreadIdx(int fd, SockT sock, size_t threadIdx) {
	std::unique_lock<std::shared_mutex> lck(mtx);
	map[fd] = { sock, threadIdx };
}
void SocketThreadMapper::removeFd(int fd) {
	std::unique_lock<std::shared_mutex> lck(mtx);
	map.erase(fd);
}