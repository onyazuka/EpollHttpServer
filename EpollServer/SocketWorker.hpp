#include "Mt/ThreadPool.hpp"
#include <optional>
#include <shared_mutex>
#include <unordered_map>

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
	void onInputData(int epollFd, int socketFd);
	void onError(int epollFd, int socketFd);
	void run();
private:
	void onCloseClient(int epollFd, int socketFd);
	bool checkFd(int fd);
	QueueT tasksQueue;
	std::jthread thread;
	static constexpr int BUF_SIZE = 1 * 1024 * 1024;
	std::unordered_map<int, std::vector<char>> sockBufs;
	//int epollFd;
	std::mutex mtx;
	SocketThreadMapper* mapper = nullptr;

};

class SocketThreadMapper {
public:
	std::optional<size_t> findThreadIdx(int fd);
	void addThreadIdx(int fd, size_t threadIdx);
	void removeFd(int fd);
private:
	std::shared_mutex mtx;
	std::unordered_map<int, size_t> map;
};
