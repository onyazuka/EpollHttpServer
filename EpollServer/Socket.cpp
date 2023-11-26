#include "Socket.hpp"
#include <cassert>
#include "ProjLogger.hpp"
#include <string.h>
#include <fcntl.h>

using namespace inet;

ISocket::~ISocket() {
	;
}

std::vector<std::shared_ptr<ISocket>> ISocket::acceptAll(bool setNonBlock) const {
	std::vector<std::shared_ptr<ISocket>> fds;
	std::shared_ptr<ISocket> fd = 0;
	do {
		fd = accept(setNonBlock);
		if (fd == nullptr) {
			break;
		}
		fds.push_back(fd);
	} while (fd != nullptr);
	return fds;
}


Socket::Socket(int __fd) 
	: _fd{ __fd }
{
	//assert(_fd >= 0);
}

Socket::~Socket() {
	//close();
}

int Socket::init() const {
	return 0;
}

std::shared_ptr<ISocket> Socket::accept(bool setNonBlock) const {
	struct sockaddr_in clientAddr;
	socklen_t clientAddrLen = sizeof(clientAddr);
	int clientFd = ::accept(_fd, (struct sockaddr*)&clientAddr, &clientAddrLen);
	if (clientFd >= 0) {
		if (setNonBlock) {
			if (fcntl(clientFd, F_SETFL, fcntl(clientFd, F_GETFL, 0) | O_NONBLOCK) < 0) {
				Log.warning(std::format("Couldn't set client socket {} as non-blocking", clientFd));
			}
		}
		return std::shared_ptr<ISocket>(new Socket(clientFd));
	}
	else {
		if (errno != EAGAIN) {
			Log.error(std::format("Failed to accept client connection: {}", strerror(errno)));
		}
		return nullptr;
	}
}

ssize_t Socket::read(std::span<char> buf) const {
	size_t nbytes = 0;
	for (;;) {
		ssize_t n = ::read(_fd, buf.data(), buf.size());
		//Log.debug(std::format("n = {}, and errno is {}", n, errno));
		if (n <= 0) {
			if (n == 0) {
				Log.debug(std::format("client socket {} has closed connection", _fd));
				return 0;
			}
			else if (errno == EAGAIN)
			{
				//Log.debug(std::format("EAGAIN on socket {}", _fd));
				return nbytes ? nbytes : -EAGAIN;
			}
			else {
				// n < 0 - error
				Log.error(std::format("error \"{}\" on socket {}", strerror(errno), _fd));
				return -errno;
			}
		}
		else {
			Log.debug(std::format("Read {} bytes from {}", n, _fd));
			nbytes += n;
			//break;
		}
	}
	return nbytes;
}

ssize_t Socket::write(std::span<char> buf) const {
	for (;;) {
		ssize_t n = ::write(_fd, buf.data(), buf.size());
		if ((n <= 0) && (errno != EAGAIN)) {
			Log.error(std::format("error writing to socket {}", _fd));
			if (n == 0) return 0;
			else return -errno;
		}
		else if (errno == EAGAIN) {
			// can't write into socket - sleep and try again
			Log.debug(std::format("Couldn't write to socket{} - EAGAIN", _fd));
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		else if (n < buf.size()) {
			Log.debug(std::format("Writtten only {} from {} bytes to {}, waiting and trying to write the rest", n, buf.size(), _fd));
			buf = std::span<char>(buf.data() + n, buf.size() - n);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			continue;
		}
		else {
			Log.debug(std::format("Write {} bytes to {}", n, _fd));
			return n;
		}
	}
	return 0;
}

int Socket::close() {
	int res = ::close(_fd);
	//_fd = -1;
	return res;
}
