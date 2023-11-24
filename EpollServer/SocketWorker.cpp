#include "SocketWorker.hpp"
#include "ProjLogger.hpp"
#include <sys/epoll.h>
#include <string.h>

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

void SocketDataHandler::onInputData(int epollFd, int fd) {
	if (!checkFd(fd)) return;
	//cout << format("Reading from epoll {} and socket {}\n", epollFd, socketFd);
	auto& buf = sockBufs[fd];
	if (buf.empty()) {
		// new connection
		// NOT reserve
		buf.resize(BUF_SIZE);
	}
	Log.debug(std::format("Handling client data {}", fd));
	for (;;) {
		ssize_t n = read(fd, buf.data(), buf.size());
		if (n <= 0 && (errno != EAGAIN)) {
			if (n == 0) {
				Log.debug(std::format("client socket {} has closed connection", fd));
			}
			else {
				// n < 0 - error
				Log.debug(std::format("error \"{}\" on socket {}", strerror(errno), fd));
			}
			onError(epollFd, fd);
			break;
		}
		else if (errno == EAGAIN)
		{
			Log.debug(std::format("EAGAIN on socket {}", fd));
		}
		else {
			Log.debug(std::format("Read {} bytes from {}", n, fd));
			//break;
		}
	}
}

void SocketDataHandler::onError(int epollFd, int fd) {
	Log.error(std::format("Closing connection from server with client {}", fd));
	onCloseClient(epollFd, fd);
}

void SocketDataHandler::onCloseClient(int epollFd, int fd) {
	if (!checkFd(fd)) return;
	epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, NULL);
	close(fd);
	mapper->removeFd(fd);
	sockBufs.erase(fd);
	
}

bool SocketDataHandler::checkFd(int fd) {
	// fd may have been already closed
	if (mapper->findThreadIdx(fd) == std::nullopt) {
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

std::optional<size_t> SocketThreadMapper::findThreadIdx(int fd) {
	std::shared_lock<std::shared_mutex> lck(mtx);
	if (auto iter = map.find(fd); iter == map.end()) {
		return std::nullopt;
	}
	else {
		return iter->second;
	}
}
void SocketThreadMapper::addThreadIdx(int fd, size_t threadIdx) {
	std::unique_lock<std::shared_mutex> lck(mtx);
	map[fd] = threadIdx;
}
void SocketThreadMapper::removeFd(int fd) {
	std::unique_lock<std::shared_mutex> lck(mtx);
	map.erase(fd);
}