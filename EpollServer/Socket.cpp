#include "Socket.hpp"
#include <cassert>
#include "ProjLogger.hpp"
#include <string.h>
#include <fcntl.h>

using namespace inet;

InputSocketBuffer::InputSocketBuffer(size_t capacity) 
	: _size{ 0 }, _capacity{capacity}
{
	assert(_capacity <= MaxCap);
	_data = std::shared_ptr<uint8_t[]>(new uint8_t[_capacity]);
}

std::span<uint8_t> InputSocketBuffer::get() {
	return std::span<uint8_t>(_data.get(), _size);
}

std::span<uint8_t> InputSocketBuffer::getTail() {
	return std::span<uint8_t>(_data.get() + _size, _capacity - _size);
}

void InputSocketBuffer::realloc(size_t newCap) {
	assert(newCap <= MaxCap && newCap > _capacity);
	std::shared_ptr<uint8_t[]> newData(new uint8_t[newCap]);
	memcpy(newData.get(), _data.get(), _size);
	_capacity = newCap;
	_data = newData;
}

void InputSocketBuffer::realloc() {
	realloc(std::min(MaxCap, std::max((size_t)0, (_capacity + MinSizeAvail) * 2)));
}

void InputSocketBuffer::clear() {
	_size = 0;
}

// clears n bytes and moves rest of bytes to the beginning of the buffer
void InputSocketBuffer::clear(size_t n) {
	assert(_size >= n);
	memmove(_data.get(), _data.get() + n, _size - n);
	_size = _size - n;
}

OutputSocketBuffer::OutputSocketBuffer() {
	;
}

OutputSocketBuffer::OutputSocketBuffer(std::string&& sdata)
	: _data{ std::move(sdata) }
{
	;
}

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

ssize_t Socket::read(InputSocketBuffer& sockBuf) const {
	size_t nbytes = 0;
	for (;;) {
		ssize_t n = sockBuf.read(&::read, _fd);
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

ssize_t Socket::write(OutputSocketBuffer& sockBuf) const {
	size_t nbytes = 0;
	for (;;) {
		ssize_t n = sockBuf.write(&::write, _fd);
		if (n <= 0) {
			if (n == 0) {
				Log.debug(std::format("error writing to socket {}", _fd));
				return 0;
			}
			else if (errno == EAGAIN) {
				// can't write into socket - sleep and try again
				//Log.debug(std::format("Couldn't write to socket {} - EAGAIN", _fd));
				//std::this_thread::sleep_for(std::chrono::milliseconds(1));
				//continue;
				return nbytes ? nbytes : -EAGAIN;
			}
			else {
				Log.error(std::format("error writing to socket {}", _fd));
				return -errno;
			}
		}
		else if (!sockBuf.finished()) {
			//Log.debug(std::format("Writtten only {} from {} bytes to {}, waiting and trying to write the rest", n, buf.size(), _fd));
			Log.debug(std::format("Write {} bytes to {}", n, _fd));
			nbytes += n;
			continue;
		}
		else {
			Log.debug(std::format("Write {} bytes to {}", n, _fd));
			nbytes += n;
			return nbytes;
		}
	}
	return nbytes;
}

int Socket::close() {
	int res = ::close(_fd);
	//_fd = -1;
	return res;
}
