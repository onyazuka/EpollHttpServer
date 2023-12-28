#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <mutex>
#include <shared_mutex>
#include "MessengerDb.hpp"

class SharedCache {
public:
	using UserT = db::User;
	using AddressBookT = db::AddressBook;
	using ChatT = db::Chat;
	using UsersV = std::unordered_set<UserT>;
	using AddressBooksV = std::unordered_set<AddressBookT>;
	using ChatsV = std::unordered_set<ChatT>;
	// key is id
	using UserIdMapT = std::unordered_map<size_t, UserT>;
	// key is username
	using UserUsernameMapT = std::unordered_map<std::string, UserT>;
	// key is whoId
	using AddressBooksWhoIdMapT = std::unordered_map<size_t, std::unordered_set<AddressBookT>>;
	// key is whoId
	using ChatsIdMapT = std::unordered_map<size_t, std::unordered_set<ChatT>>;
	// key is chatId
	//using TxtMessageT = std::unordered_map<size_t, db::TxtMessage>;
	void init(UsersV&& _users, AddressBooksV&& _addrBooks, ChatsV&& _chats);
	void userAdd(UserT user);
	bool userIsAuthentificated(size_t id, const std::string& authToken);
	std::optional<size_t> userFind(const std::string& username);
	UsersV usersFindById(const std::unordered_set<size_t>& ids);
	template<typename T, typename F>
	UsersV usersFindById(const T& data, F extractor);
	// returns authToken if user exists, else empty string
	std::pair<size_t, std::string> userLogin(const std::string& username, const std::string& pwdHash);
	void contactAdd(const AddressBookT& entry);
	AddressBooksV contactGetForId(size_t whoId);
	bool contactDelete(size_t contactId, size_t whoId);
	void chatAdd(const ChatT& chat);
	ChatsV chatsGetForId(size_t userId);
	bool chatDelete(size_t chatId, size_t userId);
private:
	UsersV vUsers;
	AddressBooksV vAddrBooks;
	ChatsV vChats;

	UserIdMapT user2Id;
	UserUsernameMapT user2Username;
	AddressBooksWhoIdMapT addrBook2WhoId;
	ChatsIdMapT chat2WhoId;
	ChatsIdMapT chat2WithId;

	std::shared_mutex usersMtx;
	std::shared_mutex addrBooksMtx;
	std::shared_mutex chatsMtx;
};

template<typename T, typename F>
SharedCache::UsersV SharedCache::usersFindById(const T& data, F extractor) {
	std::shared_lock<std::shared_mutex> lck{ usersMtx };
	UsersV users;
	for (const auto& elem : data) {
		if (auto iter = user2Id.find(extractor(elem)); iter != user2Id.end()) {
			users.insert(iter->second);
		}
	}
	return users;
}