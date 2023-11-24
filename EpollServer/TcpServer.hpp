#include <cstdio>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <unistd.h>
#include <iostream>
#include <string_view>
#include <source_location>
#include <thread>
#include "SocketWorker.hpp"

namespace inet::tcp {

	class InvalidAddrException : public std::exception {};

	class AddrInfo {
	public:
		AddrInfo(std::string_view ipv4, uint16_t port);
		inline const std::string& sAddr() const { return _sAddr; }
		inline in_addr_t addr() const { return _addr.sin_addr.s_addr; }
		inline const sockaddr_in& sockAddr() { return _addr; }
		inline uint16_t port() const { return _port; }
	private:
		std::string _sAddr;
		sockaddr_in _addr;
		uint16_t _port;
	};

	class Buffer {
	public:
		Buffer(size_t capacity);
		void supplyData(char* data, size_t sz);
		
	private:
		std::vector<char> buf;
		size_t size;
	};

	class TcpServer {
	public:

		struct Options {
			Options(bool _nonBlock);
			bool nonBlock;
		};

		TcpServer(std::string_view ipv4, uint16_t port, Options&& opts);
	private:
		int init();
		int run();
		void serverClose();
		static constexpr int MAX_LISTENING_CLIENTS = 128;
		static constexpr int MAX_EPOLL_EVENTS = 100;

		int serverFd = -1;
		int epollFd = -1;
		AddrInfo addrInfo;
		Options opts;

		SocketThreadMapper socketMapper;
		util::mt::RollingThreadPool<SocketDataHandler> threadPool;
	};


}
