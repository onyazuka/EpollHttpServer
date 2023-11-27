#include "Mt/ThreadPool.hpp"
#include <optional>
#include <shared_mutex>
#include <unordered_map>
#include "Socket.hpp"

class SocketThreadMapper;

class SocketDataHandler {
public:
	using TaskT = std::pair<std::packaged_task<std::any(std::any&)>, std::any>;
	using QueueT = util::mt::SafeQueue<TaskT>;
	SocketDataHandler(QueueT&& tasksQueue);
	SocketDataHandler(const SocketDataHandler&) = delete;
	SocketDataHandler& operator=(const SocketDataHandler&) = delete;
	SocketDataHandler(SocketDataHandler&& other) noexcept;
	SocketDataHandler& operator=(SocketDataHandler&& other) noexcept;
	~SocketDataHandler();
	void request_stop();
	void join();
	QueueT& queue();
	void setMapper(SocketThreadMapper* _mapper);
	void onInputData(int epollFd, std::shared_ptr<inet::ISocket> sock);
	void onError(int epollFd, std::shared_ptr<inet::ISocket> sock);
	void run();
private:
	void onCloseClient(int epollFd, std::shared_ptr<inet::ISocket> sock);
	bool checkFd(std::shared_ptr<inet::ISocket> sock);
	QueueT tasksQueue;
	std::jthread thread;
	std::unordered_map<int, inet::SocketBuffer> sockBufs;
	//int epollFd;
	std::mutex mtx;
	SocketThreadMapper* mapper = nullptr;

};

class SocketThreadMapper {
public:
	using SockT = std::shared_ptr<inet::ISocket>;
	std::pair<SockT, size_t> findThreadIdx(int fd);
	void addThreadIdx(int fd, SockT sock, size_t threadIdx);
	void removeFd(int fd);
private:
	std::shared_mutex mtx;
	std::unordered_map<int, std::pair<SockT, size_t>> map;
};
