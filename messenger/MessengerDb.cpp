#include "MessengerDb.hpp"
#include <format>
#include <ProjLogger.hpp>

using namespace db;
using namespace util::web::json;

User::User(size_t id, std::string username, const std::string& pwdHash, const std::string& authToken)
    : id{id}, username{username}, pwdHash{pwdHash}, authToken{authToken}
{
    ;
} 

User::User(size_t id)
    : id{id}
{
    ;
}

AddressBook::AddressBook(size_t id, size_t whoId, size_t withId)
    : id{ id }, whoId{ whoId }, withId{ withId }
{
    ;
}

AddressBook::AddressBook(size_t id)
    : id{ id }
{
    ;
}

Chat::Chat() 
{
    ;
}

Chat::Chat(size_t id, size_t whoId, size_t withId)
    : id{id}, whoId{whoId}, withId{withId}
{
    ;
}

Chat::Chat(size_t id) 
    : id{id}
{
    ;
}

TxtMessage::TxtMessage(size_t id, size_t chatId, size_t whoId, const std::string& message, size_t timestamp)
    : id{ id }, chatId{ chatId }, whoId{ whoId }, message{ message }, timestamp{ timestamp }
{
    ;
}

TxtMessage::TxtMessage(size_t id) 
    : id{id}
{
    ;
}

util::web::json::ObjNode User::toObjNode() const {
    return ObjNode({
        {"id", (int64_t)id},
        {"username", username},
        {"pwdHash", pwdHash},
        {"authToken", authToken}
        });
}

util::web::json::ObjNode AddressBook::toObjNode() const {
    return ObjNode({
       {"id", (int64_t)id},
       {"whoId", (int64_t)whoId},
       {"withId", (int64_t)withId}
        });
}

util::web::json::ObjNode Chat::toObjNode() const {
    return ObjNode({
       {"id", (int64_t)id},
       {"whoId", (int64_t)whoId},
       {"withId", (int64_t)withId}
        });
}

util::web::json::ObjNode TxtMessage::toObjNode() const {
    return ObjNode({
       {"id", (int64_t)id},
       {"chatId", (int64_t)chatId},
       {"whoId", (int64_t)whoId},
        {"message", message},
        {"ts", (int64_t)timestamp}
        });
}

MessengerDb::MessengerDb(const std::string& url, const std::string& username, const std::string& pwd, const std::string& schema)
	: db{new DbMysql()}
{
    db->connect(url, username, pwd);
    db->setSchema(schema);
	//db->connect("tcp://127.0.0.1:3306", "onyazuka", "5051");
	//db->setSchema("messenger");
}

std::pair<MessengerDb::Error, std::optional<size_t>> MessengerDb::registerUser(const std::string& username, const std::string& pwdHash, const std::string& authToken) {
    try {
        int sz = db->modify(std::format("insert into User values (NULL,'{}','{}','{}')", username, pwdHash, authToken));
        if (sz) {
            db->query("select LAST_INSERT_ID()");
            return { Error::Ok, db->result<uint64_t>() };
        }
        else {
            return { Error::InvalidQuery, std::nullopt };
        }
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, std::nullopt };
    }
}

std::pair<MessengerDb::Error, bool> MessengerDb::loginUser(size_t id, const std::string& authToken) {
    try {
        db->query(std::format("select count(*) from User where id={} and authToken='{}'", id, authToken));
        return { Error::Ok, db->result<uint64_t>() };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, false };
    }
}

std::pair<MessengerDb::Error, std::vector<User>> MessengerDb::getUsers() {
    try {
        db->query("select * from User");
        return { Error::Ok, vecTuples2vecStructs<User>(db->result<size_t, std::string, std::string, std::string>({1,2,3,4})) };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, {} };
    }
}

std::pair<MessengerDb::Error, std::optional<size_t>> MessengerDb::addContactToAddressBook(size_t whoId, size_t withId) {
    if (whoId == withId) {
        return { Error::InvalidQuery, std::nullopt };
    }
    try {
        int sz = db->modify(std::format("insert into AddressBook values (NULL, {}, {})", whoId, withId));
        if (sz) {
            db->query("select LAST_INSERT_ID()");
            return { Error::Ok, db->result<uint64_t>() };
        }
        else {
            return { Error::InvalidQuery, std::nullopt };
        }
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, std::nullopt };
    }
}

std::pair<MessengerDb::Error, std::vector<AddressBook>> MessengerDb::getAddressBooks() {
    try {
        db->query("select * from AddressBook");
        return { Error::Ok, vecTuples2vecStructs<AddressBook>(db->result<size_t, size_t, size_t>({1,2,3})) };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, {} };
    }
}

std::pair<MessengerDb::Error, std::vector<AddressBook>> MessengerDb::getContactsFromAddressBook(size_t forWhoId) {
    try {
        db->query(std::format("select * from AddressBook where whoId={}", forWhoId));
        return { Error::Ok, vecTuples2vecStructs<AddressBook>(db->result<size_t,size_t,size_t>({1,2,3})) };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, {} };
    }
}

std::pair<MessengerDb::Error, bool> MessengerDb::deleteContactFromAddressBook(size_t whoId, size_t withId) {
    try {
        int sz = db->modify(std::format("delete from AddressBook where whoId={} and withId={}", whoId, withId));
        if (sz) return { Error::Ok, true };
        else return { Error::NotExists, false };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, false };
    }
}

std::pair<MessengerDb::Error, bool> MessengerDb::deleteContactFromAddressBook(size_t id) {
    try {
        int sz = db->modify(std::format("delete from AddressBook where id={}", id));
        if (sz) return { Error::Ok, true };
        else return { Error::NotExists, false };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, false };
    }
}

std::pair<MessengerDb::Error, std::optional<size_t>> MessengerDb::addChat(size_t whoId, size_t withId) {
    if (whoId == withId) {
        return { Error::InvalidQuery, std::nullopt };
    }
    // chats {whoId, withId} and {withId, whoId} are equal, so sorting, then adding
    if (withId < whoId) {
        std::swap(whoId, withId);
    }
    try {
        //db->query(std::format("select count(*) from AddressBook where (whoId={} and withId={}) or (whoId={} and withId={})", whoId, withId, withId, whoId));
        // whoId and withId should be mutual contacts in AddressBook
        //if (db->result<uint64_t>() != 2) {
          //  return { Error::InvalidQuery, std::nullopt };
        //}
        int sz = db->modify(std::format("insert into Chat values (NULL, {}, {})", whoId, withId));
        if (sz) {
            db->query("select LAST_INSERT_ID()");
            return { Error::Ok, db->result<uint64_t>() };
        }
        else {
            return { Error::InvalidQuery, std::nullopt };
        }
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, std::nullopt };
    }
}

std::pair<MessengerDb::Error, std::vector<Chat>> MessengerDb::getChatsForId(size_t forWhoId) {
    try {
        db->query(std::format("select * from Chat where whoId={}", forWhoId));
        std::vector<std::tuple<size_t, size_t, size_t>> res1 = db->result<size_t, size_t, size_t>({ 1,2,3 });
        db->query(std::format("select * from Chat where withId={}", forWhoId));
        std::vector<std::tuple<size_t, size_t, size_t>> res2 = db->result<size_t, size_t, size_t>({ 1,2,3 });
        res1.insert(res1.end(), res2.begin(), res2.end());
        return { Error::Ok, vecTuples2vecStructs<Chat>(std::move(res1)) };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, {} };
    }
}

std::pair<MessengerDb::Error, std::vector<Chat>> MessengerDb::getChats() {
    try {
        db->query("select * from Chat");
        return { Error::Ok, vecTuples2vecStructs<Chat>(db->result<size_t,size_t,size_t>({1,2,3})) };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, {} };
    }
}

std::pair<MessengerDb::Error, bool> MessengerDb::deleteChat(size_t whoId, size_t withId) {
    if (withId < whoId) {
        std::swap(whoId, withId);
    }
    try {
        int sz = db->modify(std::format("delete from Chat where whoId={} and withId={}", whoId, withId));
        if (sz) return { Error::Ok, true };
        else return { Error::NotExists, false };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, false };
    }
}

std::pair<MessengerDb::Error, bool> MessengerDb::deleteChat(size_t id) {
    try {
        int sz = db->modify(std::format("delete from Chat where id={}", id));
        if (sz) return { Error::Ok, true };
        else return { Error::NotExists, false };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, false };
    }
}

std::pair<MessengerDb::Error, std::optional<size_t>> MessengerDb::addTxtMessage(size_t chatId, size_t whoId, const std::string& text, size_t timestamp) {
    try {
        // inserting only if whoId matches to chatId whoId or withId
        //std::string query = std::format("insert into TxtMessage select NULL, {}, {}, '{}', NOW() where (select COUNT(*) from Chat where chatId={} and whoId={}) > 0 or (select COUNT(*) from Chat where chatId={} and withId={}) > 0", chatId, whoId, SEC(text), chatId, whoId, chatId, whoId);
        int sz = db->modify(std::format("insert into TxtMessage select NULL, {}, {}, '{}', {} where (select COUNT(*) from Chat where id={} and whoId={}) > 0 or (select COUNT(*) from Chat where id={} and withId={}) > 0", chatId, whoId, SEC(text), timestamp, chatId, whoId, chatId, whoId));
        if (sz) {
            db->query("select LAST_INSERT_ID()");
            return { Error::Ok, db->result<uint64_t>() };
        }
        else {
            return { Error::InvalidQuery, std::nullopt };
        }
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, std::nullopt };
    }
}

std::pair<MessengerDb::Error, bool> MessengerDb::deleteTxtMessage(size_t id) {
    try {
        int sz = db->modify(std::format("delete from TxtMessage where id={}", id));
        if (sz) return { Error::Ok, true };
        else return { Error::NotExists, false };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, false };
    }
}

std::pair<MessengerDb::Error, std::vector<TxtMessage>> MessengerDb::getTxtMessagesForChat(size_t chatId) {
    try {
        db->query(std::format("select * from TxtMessage where chatId={}", chatId));
        return { Error::Ok, vecTuples2vecStructs<TxtMessage>(db->result<size_t, size_t, size_t, std::string, size_t>({ 1,2,3,4,5 })) };
    }
    catch (std::exception& ex) {
        Log.error(ex.what());
        return { Error::InvalidQuery, {} };
    }
}

void MessengerDb::createTables() {
    db->modify("CREATE TABLE User (id bigint unsigned NOT NULL AUTO_INCREMENT,username varchar(64) NOT NULL,pwdHash char(64) NOT NULL,authToken char(64) NOT NULL,PRIMARY KEY(id),UNIQUE KEY username (username))");
    db->modify("CREATE TABLE AddressBook (id bigint unsigned NOT NULL AUTO_INCREMENT,whoId bigint unsigned NOT NULL,withId bigint unsigned NOT NULL,PRIMARY KEY(id),UNIQUE KEY unique_entry (whoId,withId),KEY withId (withId),CONSTRAINT AddressBook_ibfk_1 FOREIGN KEY(whoId) REFERENCES User (id) ON DELETE CASCADE ON UPDATE CASCADE,CONSTRAINT AddressBook_ibfk_2 FOREIGN KEY(withId) REFERENCES User (id) ON DELETE CASCADE ON UPDATE CASCADE)");
    db->modify("CREATE TABLE Chat (id bigint unsigned NOT NULL AUTO_INCREMENT,whoId bigint unsigned NOT NULL,withId bigint unsigned NOT NULL,PRIMARY KEY(id),UNIQUE KEY unique_entry (whoId,withId),KEY withId (withId),CONSTRAINT Chat_ibfk_1 FOREIGN KEY(whoId) REFERENCES User (id) ON DELETE CASCADE ON UPDATE CASCADE,CONSTRAINT Chat_ibfk_2 FOREIGN KEY(withId) REFERENCES User (id) ON DELETE CASCADE ON UPDATE CASCADE)");
    db->modify("CREATE TABLE TxtMessage (id bigint unsigned NOT NULL AUTO_INCREMENT,chatId bigint unsigned NOT NULL,whoId bigint unsigned NOT NULL,message text NOT NULL,ts timestamp NOT NULL,PRIMARY KEY(id),KEY chatId (chatId),KEY whoId (whoId),CONSTRAINT TxtMessage_ibfk_1 FOREIGN KEY(chatId) REFERENCES Chat (id) ON DELETE CASCADE ON UPDATE CASCADE,CONSTRAINT TxtMessage_ibfk_2 FOREIGN KEY(whoId) REFERENCES User (id) ON DELETE CASCADE ON UPDATE CASCADE)");
}

void MessengerDb::deleteTables() {
    db->modify("drop table TxtMessage");
    db->modify("drop table Chat");
    db->modify("drop table AddressBook");
    db->modify("drop table User");
}

void MessengerDb::cleanDb() {
	db->modify("delete from User");
	db->modify("delete from AddressBook");
	db->modify("delete from Chat");
	db->modify("delete from TxtMessage");
}