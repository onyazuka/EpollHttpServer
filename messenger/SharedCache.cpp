#include "SharedCache.hpp"

void SharedCache::init(UsersV&& _users, AddressBooksV&& _addrBooks, ChatsV&& _chats) {
	vUsers = std::move(_users);
	vAddrBooks = std::move(_addrBooks);
	vChats = std::move(_chats);

	for (auto& user : vUsers) {
		user2Id.emplace(user.id, user);
		user2Username.emplace(user.username, user);
	}
	for (auto& addrBook : vAddrBooks) {
		addrBook2WhoId[addrBook.whoId].insert(addrBook);
	}
	for (auto& chat : vChats) {
		chat2WhoId[chat.whoId].insert(chat);
		chat2WithId[chat.withId].insert(chat);
	}

}

void SharedCache::userAdd(UserT _user) {
	std::unique_lock<std::shared_mutex> lck{ usersMtx };
	vUsers.insert(_user);
	user2Id.emplace(_user.id, _user);
	user2Username.emplace(_user.username, _user);
}

bool SharedCache::userIsAuthentificated(size_t id, const std::string& authToken) {
	std::shared_lock<std::shared_mutex> lck{ usersMtx };
	if (auto iter = user2Id.find(id); iter == user2Id.end()) {
		return false;
	}
	else {
		if (iter->second.authToken != authToken) {
			return false;
		}
		return true;
	}
}

std::pair<size_t, std::string> SharedCache::userLogin(const std::string& username, const std::string& pwdHash) {
	std::shared_lock<std::shared_mutex> lck{ usersMtx };
	if (auto iter = user2Username.find(username); iter == user2Username.end()) {
		return { 0, "" };
	}
	else {
		if (iter->second.pwdHash != pwdHash) {
			return { 0, "" };
		}
		return { iter->second.id, iter->second.authToken };
	}
}

void SharedCache::contactAdd(const AddressBookT& _entry) {
	std::unique_lock<std::shared_mutex> lck{ addrBooksMtx };
	vAddrBooks.insert(_entry);
	addrBook2WhoId[_entry.whoId].insert(_entry);
}

SharedCache::AddressBooksV SharedCache::contactGetForId(size_t whoId) {
	std::shared_lock<std::shared_mutex> lck{ addrBooksMtx };
	if (auto iter = addrBook2WhoId.find(whoId); iter != addrBook2WhoId.end()) {
		return iter->second;
	}
	return {};
}

bool SharedCache::contactDelete(size_t contactId, size_t whoId) {
	std::unique_lock<std::shared_mutex> lck{ addrBooksMtx };
	AddressBookT dummyAddrBook(contactId);
	if (!vAddrBooks.erase(dummyAddrBook)) {
		return false;
	}
	if (auto iter = addrBook2WhoId.find(whoId); iter == addrBook2WhoId.end()) {
		return false;
	}
	else {
		if (!iter->second.erase(dummyAddrBook)) {
			return false;
		}
	}
	return true;
}

void SharedCache::chatAdd(const ChatT& chat) {
	std::unique_lock<std::shared_mutex> lck{ chatsMtx };
	vChats.insert(chat);
	chat2WhoId[chat.whoId].insert(chat);
	chat2WithId[chat.withId].insert(chat);
}

SharedCache::ChatsV SharedCache::chatsGetForId(size_t userId) {
	std::shared_lock<std::shared_mutex> lck{ chatsMtx };
	auto iWho = chat2WhoId.end();
	auto iWith = chat2WithId.end();
	iWho = chat2WhoId.find(userId);
	iWith = chat2WithId.find(userId);
	ChatsV res;
	if (iWho != chat2WhoId.end()) {
		res.insert(iWho->second.begin(), iWho->second.end());
	}
	if (iWith != chat2WithId.end()) {
		res.insert(iWith->second.begin(), iWith->second.end());
	}
	return res;
}

bool SharedCache::chatDelete(size_t chatId, size_t userId) {
	std::unique_lock<std::shared_mutex> lck{ chatsMtx };
	ChatT dummyChat(chatId);
	if (!vChats.erase(dummyChat)) {
		return false;
	}
	if (auto iter = chat2WhoId.find(userId); iter != chat2WhoId.end()) {
		iter->second.erase(dummyChat);
	}
	if (auto iter = chat2WithId.find(userId); iter != chat2WhoId.end()) {
		iter->second.erase(dummyChat);
	}
	return true;
}
