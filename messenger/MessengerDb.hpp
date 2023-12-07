#pragma once
#include "DbMysql.hpp"
#include <optional>
#include <cstdint>
#include "Json.hpp"
#include "Utils_String.hpp"

namespace db {

#define SEC(s) (util::string::escapeUnsafe(s))

	template<typename T, typename... Args>
	std::vector<T> vecTuples2vecStructs(std::vector<std::tuple<Args...>>&& vt) {
		std::vector<T> res;
		res.reserve(vt.size());
		for (auto& t : vt) {
			res.push_back(std::move(std::make_from_tuple<T>(t)));
		}
		return res;
	}

	struct User {
		User(size_t id, std::string username, const std::string& pwdHash, const std::string& authToken);
		User(size_t id);
		size_t id;
		std::string username;
		std::string pwdHash;
		std::string authToken;
		util::web::json::ObjNode toObjNode() const;
		inline bool operator==(const User& other) const { return id == other.id; }
	};

	struct AddressBook {
		AddressBook(size_t id, size_t whoId, size_t withId);
		AddressBook(size_t id);
		size_t id;
		size_t whoId;
		size_t withId;
		util::web::json::ObjNode toObjNode() const;
		inline bool operator==(const AddressBook& other) const { return id == other.id; }
	};

	struct Chat {
		Chat(size_t id, size_t whoId, size_t withId);
		Chat(size_t id);
		size_t id;
		size_t whoId;
		size_t withId;
		util::web::json::ObjNode toObjNode() const;
		inline bool operator==(const Chat& other) const { return id == other.id; }
	};

	struct TxtMessage {
		TxtMessage(size_t id, size_t chatId, size_t whoId, const std::string& message, const std::string& timestamp);
		TxtMessage(size_t id);
		size_t id;
		size_t chatId;
		size_t whoId;
		std::string message;
		std::string timestamp;
		util::web::json::ObjNode toObjNode() const;
		inline bool operator==(const TxtMessage& other) const { return id == other.id; }
	};


	class MessengerDb {
	public:

		enum class Error {
			Ok,
			InvalidQuery,
			NotExists
		};


		MessengerDb(const std::string& url, const std::string& username, const std::string& pwd, const std::string& schema);

		// returns id of new user
		std::pair<Error, std::optional<size_t>> registerUser(const std::string& username, const std::string& pwdHash, const std::string& authToken);
		// returns true if user exists
		std::pair<Error, bool> loginUser(size_t id, const std::string& authToken);
		// returns tuples describing users
		std::pair<Error, std::vector<User>> getUsers();
		// returns id of new contact
		std::pair<Error, std::optional<size_t>> addContactToAddressBook(size_t whoId, size_t withId);
		// returns all address books
		std::pair<Error, std::vector<AddressBook>> getAddressBooks();
		// returns vector of address book fields
		std::pair<Error, std::vector<AddressBook>> getContactsFromAddressBook(size_t forWhoId);
		// returns true if contact was deleted
		std::pair<Error, bool> deleteContactFromAddressBook(size_t whoId, size_t withId);
		// returns true if contact was deleted
		std::pair<Error, bool> deleteContactFromAddressBook(size_t id);
		// returns id of new chat
		std::pair<Error, std::optional<size_t>> addChat(size_t whoId, size_t withId);
		// returns vector of chat fields
		std::pair<Error, std::vector<Chat>> getChatsForId(size_t forWhoId);
		// returns all chats
		std::pair<Error, std::vector<Chat>> getChats();
		// returns true if chat was deleted
		std::pair<Error, bool> deleteChat(size_t whoId, size_t withId);
		// returns true if chat was deleted
		std::pair<Error, bool> deleteChat(size_t id);
		// returns id of new message
		std::pair<Error, std::optional<size_t>> addTxtMessage(size_t chatId, size_t whoId, const std::string& text);
		// returns true if txtMessage was deleted
		std::pair<Error, bool> deleteTxtMessage(size_t id);
		// returns vector of txt message fields
		std::pair<Error, std::vector<TxtMessage>> getTxtMessagesForChat(size_t chatId);

		void createTables();
		void deleteTables();
		void cleanDb();
	private:
		std::unique_ptr<db::IDb> db;
	};

}

namespace std {
	template <> struct hash<db::User>
	{
		size_t operator()(const db::User& x) const
		{
			return std::hash<size_t>()(x.id);
		}
	};
}

namespace std {
	template <> struct hash<db::AddressBook>
	{
		size_t operator()(const db::AddressBook& x) const
		{
			return std::hash<size_t>()(x.id);
		}
	};
}

namespace std {
	template <> struct hash<db::Chat>
	{
		size_t operator()(const db::Chat& x) const
		{
			return std::hash<size_t>()(x.id);
		}
	};
}

namespace std {
	template <> struct hash<db::TxtMessage>
	{
		size_t operator()(const db::TxtMessage& x) const
		{
			return std::hash<size_t>()(x.id);
		}
	};
}