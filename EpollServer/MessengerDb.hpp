#include "DbMysql.hpp"
#include <optional>
#include "Utils_String.hpp"

namespace db {

#define SEC(s) (util::string::escapeUnsafe(s))

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
		// returns id of new contact
		std::pair<Error, std::optional<size_t>> addContactToAddressBook(size_t whoId, size_t withId);
		// returns vector of address book fields
		std::pair<Error, std::vector<std::tuple<size_t, size_t, size_t>>> getContactsFromAddressBook(size_t forWhoId);
		// returns true if contact was deleted
		std::pair<Error, bool> deleteContactFromAddressBook(size_t whoId, size_t withId);
		// returns true if contact was deleted
		std::pair<Error, bool> deleteContactFromAddressBook(size_t id);
		// returns id of new chat
		std::pair<Error, std::optional<size_t>> addChat(size_t whoId, size_t withId);
		// returns vector of chat fields
		std::pair<Error, std::vector<std::tuple<size_t, size_t, size_t>>> getChats(size_t forWhoId);
		// returns true if chat was deleted
		std::pair<Error, bool> deleteChat(size_t whoId, size_t withId);
		// returns true if chat was deleted
		std::pair<Error, bool> deleteChat(size_t id);
		// returns id of new message
		std::pair<Error, std::optional<size_t>> addTxtMessage(size_t chatId, size_t whoId, const std::string& text);
		// returns true if txtMessage was deleted
		std::pair<Error, bool> deleteTxtMessage(size_t id);
		// returns vector of txt message fields
		std::pair<Error, std::vector<std::tuple<size_t, size_t, size_t, std::string, std::string>>> getTxtMessagesForChat(size_t chatId);

		void createTables();
		void deleteTables();
		void cleanDb();
	private:
		std::unique_ptr<db::IDb> db;
	};

}