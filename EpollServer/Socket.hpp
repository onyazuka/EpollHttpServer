#pragma once
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <vector>
#include <span>
#include <memory>
#include <unordered_map>

namespace inet {

	class SocketBuffer {
	public:
		SocketBuffer(size_t capacity = DefaultCap);
		std::span<uint8_t> get();
		void clear();
	private:
		void realloc(size_t newCap);
		void realloc();
		std::shared_ptr<uint8_t[]> data;
		size_t size = 0;
		size_t capacity = 0;
		static constexpr size_t MinSizeAvail = 100;
		static constexpr size_t DefaultCap = 10 * 1024;
		static constexpr size_t MaxCap = 1 * 1024 * 1024;
	};
	
	class ISocket {
	public:
		virtual ~ISocket();
		virtual int init() const = 0;
		virtual std::shared_ptr<ISocket> accept(bool setNonBlock) const = 0;
		std::vector<std::shared_ptr<ISocket>> acceptAll(bool setNonBlock) const;
		virtual ssize_t read(SocketBuffer& buf) const = 0;
		virtual ssize_t write(std::span<char> buf) const = 0;
		//virtual int shutdown(int flags) = 0;
		virtual int close() = 0;
		virtual int fd() const = 0;
	};

	class Socket : public ISocket {
	public:
		Socket(int _fd);
		~Socket();
		int init() const override;
		std::shared_ptr<ISocket> accept(bool setNonBlock) const override;
		ssize_t read(SocketBuffer& buf) const override;
		ssize_t write(std::span<char> buf) const override;
		//int shutdown(int flags);
		int close() override;
		inline int fd() const override {
			return _fd;
		}
	private:
		int _fd = -1;
	};

}

template <>
struct std::hash<std::shared_ptr<inet::ISocket>>
{
	std::size_t operator()(std::shared_ptr<inet::ISocket> ptr) const
	{
		return std::hash<int>()(ptr->fd());
	}
};
