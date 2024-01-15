#include "Api.hpp"
#include <format>
#include "ProjLogger.hpp"
#include "profiling.hpp"
#include "EventBroker.hpp"

using namespace util::web::http;
using namespace util::web::json;
using namespace util::crypto;
using namespace util::prof;
using namespace db;

#define NotAuthGuard size_t userId = 0; bool auth = false; if (std::tie(auth, userId) = userIsAuthenticated(request); !auth) return response(request, 403);

Api::Api(std::unique_ptr<db::MessengerDb> pdb)
    : db{std::move(pdb)}
{
    usernameByIdExtractor = [](const auto& user) {
        return std::make_pair(std::to_string(user.id), user.username);
        };

    onInit();

    // test - TODO delete
    HttpServer::get().registerRoute("/echo", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return echo(request, cbMsgFn); });
    HttpServer::get().registerRoute("/hello", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return hello(request, cbMsgFn); });
    
    HttpServer::get().registerRoute("/*", Method::OPTIONS, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return onOptions(request, cbMsgFn); });
    HttpServer::get().registerRoute("/user/find*", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return userFind(request, cbMsgFn); });
    HttpServer::get().registerRoute("/user/login", Method::POST, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return userRegisterOrLogin(request, cbMsgFn); });
    HttpServer::get().registerRoute("/user/logout", Method::POST, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return userLogout(request, cbMsgFn); });
    HttpServer::get().registerRoute("/contact", Method::POST, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return contactAdd(request, cbMsgFn); });
    HttpServer::get().registerRoute("/contact", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return contactsGetForId(request, cbMsgFn); });
    HttpServer::get().registerRoute("/contact", Method::DELETE, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return contactDelete(request, cbMsgFn); });
    HttpServer::get().registerRoute("/chat", Method::POST, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return chatAdd(request, cbMsgFn); });
    HttpServer::get().registerRoute("/chat", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return chatsGetForId(request, cbMsgFn); });
    HttpServer::get().registerRoute("/chat", Method::DELETE, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return chatDelete(request, cbMsgFn); });
    HttpServer::get().registerRoute("/message", Method::POST, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return txtMessageAdd(request, cbMsgFn); });
    HttpServer::get().registerRoute("/message*", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return txtMessagesGetForChatId(request, cbMsgFn); });
    HttpServer::get().registerRoute("/storage*", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return storageGet(request, cbMsgFn); });
    HttpServer::get().registerRoute("/events", Method::GET, [this](const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn) { return eventsSubscribe(request, cbMsgFn); });
}

util::web::http::HttpResponse Api::onOptions(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    HttpHeaders headers;
    headers.add("Access-Control-Allow-Origin", request.headers.find("Origin"));
    headers.add("Access-Control-Allow-Headers", request.headers.find("Access-Control-Request-Headers"));
    headers.add("Access-Control-Allow-Methods", request.headers.find("Access-Control-Request-Method"));
    return response(request, 200, std::move(headers));
}

util::web::http::HttpResponse Api::userRegisterOrLogin(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        auto json = JsonDecoder().decode(request.body);
        auto username = json.as<std::string>("username");
        auto pwdHash = json.as<std::string>("pwdHash");
        std::string authToken;
        size_t userId = 0;
        // register
        if (std::tie(userId, authToken) = sharedCache.userLogin(username, pwdHash); authToken.empty()) {
            authToken = generateAuthToken(username, pwdHash);
            auto [err, optNewUserId] = db->registerUser(username, pwdHash, authToken);
            if ((err != MessengerDb::Error::Ok) || (!optNewUserId.has_value())) {
                throw std::invalid_argument(std::format("can't registed user {}: err = {}", username, (int)err));
            }
            userId = optNewUserId.value();
            sharedCache.userAdd({ userId, username, pwdHash, authToken });
        }
        // login
        else {
            ;
        }
        HttpHeaders headers;
        headers.add("Set-Cookie", std::format("userId={}; path=/; SameSite=None; Secure\nSet-Cookie:authToken={}; path=/; SameSite=None; Secure", userId, authToken));
        return response(request, 200, std::move(headers));
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::userLogout(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    //if (!isUserAuthenticated(request)) return notAuth(request);
    NotAuthGuard;
    HttpHeaders headers;
    headers.add("Set-Cookie", std::format("userId=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT\nSet-Cookie:authToken=deleted; path=/; expires=Thu, 01 Jan 1970 00:00:00 GMT"));
    return response(request, 200, std::move(headers));
}

util::web::http::HttpResponse Api::userFind(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        NotAuthGuard;
        auto username = request.query.find("username");
        if (username.empty()) {
            throw std::logic_error("request with empty username");
        }
        auto id = sharedCache.userFind(username);
        if (!id.has_value()) {
            return response(request, 404);
        }
        HttpHeaders headers;
        headers.add("Content-Type", "application/json");
        return response(request, 200, std::move(headers), JsonEncoder().encode(Node(ValNode((int64_t)id.value()))));
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::contactAdd(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto withId = json.as<size_t>("withId");
        auto [err, optAddrBookEntryId] = db->addContactToAddressBook(userId, withId);
        if (err != db::MessengerDb::Error::Ok) {
            throw std::invalid_argument(std::format("can't add contact for id {}: err = {}", userId, (int)err));
        }
        // automatically adding opposite contact
        auto [err2, optAddrBookEntryId2] = db->addContactToAddressBook(withId, userId);
        if (err2 != db::MessengerDb::Error::Ok) {
            throw std::invalid_argument(std::format("can't add contact for id {}: err = {}", withId, (int)err));
        }
        sharedCache.contactAdd({ optAddrBookEntryId.value(), userId, withId });
        sharedCache.contactAdd({ optAddrBookEntryId2.value(), withId, userId });
        HttpHeaders headers;
        headers.add("Content-Type", "application/json");
        db::AddressBook addrBook(optAddrBookEntryId.value(), userId, withId);
        return response(request, 200, std::move(headers), JsonEncoder().encode(Node(addrBook.toObjNode())));
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::contactsGetForId(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    NotAuthGuard;
    auto contacts = sharedCache.contactGetForId(userId);
    ObjNode resContacts = ObjNode::makeFrom(contacts, [](const auto& contact) { return std::make_pair(std::to_string(contact.id), contact.toObjNode()); });
    auto users = sharedCache.usersFindById(contacts, [](const SharedCache::AddressBookT& contact) { return std::vector<size_t>{contact.withId, contact.whoId}; });
    ObjNode resUsers = ObjNode::makeFrom(users, usernameByIdExtractor);
    ObjNode res({
        {"contacts", std::move(resContacts)},
        {"users", std::move(resUsers)}
        });
    HttpHeaders headers;
    headers.add("Content-Type", "application/json");
    return response(request, 200, std::move(headers), JsonEncoder().encode(Node(res)));
}

util::web::http::HttpResponse Api::contactDelete(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto contactId = json.as<size_t>("id");
        if (!sharedCache.contactDelete(contactId, userId)) {
            return response(request, 400);
        }
        auto [err, ok] = db->deleteContactFromAddressBook(contactId);
        if (err != db::MessengerDb::Error::Ok || !ok) {
            return response(request, 400);
        }
        return response(request, 200);
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::chatAdd(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto withId = json.as<size_t>("withId");
        auto [err, optChatEntryId] = db->addChat(userId, withId);
        if (err != db::MessengerDb::Error::Ok) {
            throw std::invalid_argument(std::format("can't add chat for id {}: err = {}", userId, (int)err));
        }
        sharedCache.chatAdd({ optChatEntryId.value(), userId, withId });
        HttpHeaders headers;
        headers.add("Content-Type", "application/json");
        db::Chat chat(optChatEntryId.value(), userId, withId);
        return response(request, 200, std::move(headers), JsonEncoder().encode(Node(chat.toObjNode())));
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::chatsGetForId(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    NotAuthGuard;
    auto chats = sharedCache.chatsGetForId(userId);
    ObjNode resChats = ObjNode::makeFrom(chats, [](const auto& chat) { return std::make_pair(std::to_string(chat.id), chat.toObjNode()); });
    auto users = sharedCache.usersFindById(chats, [](const SharedCache::ChatT& chat) { return std::vector<size_t>{chat.withId, chat.whoId}; });
    ObjNode resUsers = ObjNode::makeFrom(users, usernameByIdExtractor);
    ObjNode res({
        {"chats", std::move(resChats)},
        {"users", std::move(resUsers)}
        });
    HttpHeaders headers;
    headers.add("Content-Type", "application/json");
    return response(request, 200, std::move(headers), JsonEncoder().encode(Node(res)));
}

util::web::http::HttpResponse Api::chatDelete(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto chatId = json.as<size_t>("id");
        if (!sharedCache.chatDelete(chatId, userId)) {
            return response(request, 400);
        }
        auto [err, ok] = db->deleteChat(chatId);
        if (err != db::MessengerDb::Error::Ok || !ok) {
            return response(request, 400);
        }
        return response(request, 200);
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::txtMessageAdd(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto chatId = json.as<size_t>("chatId");
        std::string message = json.as<std::string>("message");
        auto chats = sharedCache.chatsGetForId(userId);
        db::Chat curChat;
        if (auto iter = chats.find(db::Chat(chatId)); iter == chats.end()) {
            return response(request, 403);
        }
        else {
            curChat = *iter;
        }
        size_t ts = tsMs();
        auto [err, optId] = db->addTxtMessage(chatId, userId, message, ts);
        if (err != db::MessengerDb::Error::Ok) {
            return response(request, 400);
        }
        HttpHeaders headers;
        headers.add("Content-Type", "application/json");
        db::TxtMessage msg(optId.value(), chatId, userId, message, ts);
        auto body = JsonEncoder().encode(Node(msg.toObjNode()));
        EventBroker::get().emitEvent(userId == curChat.whoId ? curChat.withId : curChat.whoId, "data: " + body + "\r\n\r\n");
        auto resp = response(request, 200, std::move(headers), std::move(body));
        return resp;
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::txtMessagesGetForChatId(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    try {
        NotAuthGuard;
        auto sChatId = request.query.find("chatId");
        if (sChatId.empty()) {
            return response(request, 400);
        }
        size_t chatId = std::stoull(sChatId);
        if (!sharedCache.chatsGetForId(userId).contains(db::Chat(chatId))) {
            return response(request, 403);
        }
        auto [err, messages] = db->getTxtMessagesForChat(chatId);
        if (err != db::MessengerDb::Error::Ok) {
            return response(request, 400);
        }
        ObjNode resMessages = ObjNode::makeFrom(messages, [](const auto& message) { return std::make_pair(std::to_string(message.id), message.toObjNode()); });
        auto users = sharedCache.usersFindById(messages, [](const db::TxtMessage& message) { return std::vector<size_t>{message.whoId}; });
        ObjNode resUsers = ObjNode::makeFrom(users, usernameByIdExtractor);
        ObjNode res({
            {"messages", std::move(resMessages)},
            {"users", std::move(resUsers)}
            });
        HttpHeaders headers;
        headers.add("Content-Type", "application/json");
        return response(request, 200, std::move(headers), JsonEncoder().encode(Node(res)));
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::storageGet(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    NotAuthGuard;
    auto resp = HttpServer::get().getEntireFile(request.url, request);
    return response(request, resp.status, std::move(resp.headers), std::move(resp.body));
    //return response(request, 200, {}, "<html><body><h1>Hi, storage!</h1></body></html>");
}

util::web::http::HttpResponse Api::eventsSubscribe(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cb) {
    NotAuthGuard;
    HttpHeaders headers;
    headers.add("Content-Type", "text/event-stream");
    headers.add("Connection", "keep-alive");
    headers.add("Cache-Control", "no-store");
    headers.add("Access-Control-Allow-Credentials", "true");
    headers.borrow(request.headers, "Origin", "", "Access-Control-Allow-Origin");
    EventBroker::get().registerProducerAndHandler(userId, cb);
    /*std::thread([this, userId, request, headers]() {
        for(size_t i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(10));
            EventBroker::get().emitEvent(userId, std::string("data: Hello EventSource\r\n\r\n"));
        }
        }).detach();*/
    return HttpResponse(200, std::move(headers), "HTTP/1.1 200\r\n", request.headers);
    //return response(request, 200, std::move(headers));
}

util::web::http::HttpResponse Api::response(const util::web::http::HttpRequest& request, size_t code, util::web::http::HttpHeaders&& headers, std::string&& body) {
    Log.info(std::format("Sending response {}", code));
    headers.add("Content-Length", body.size());
    headers.borrow(request.headers, "Connection");
    headers.borrow(request.headers, "Origin", "", "Access-Control-Allow-Origin");
    headers.add("Access-Control-Allow-Credentials", "true");
    return HttpResponse(
        code,
        std::move(headers),
        std::move(body),
        request.headers
    );
}

util::web::http::HttpResponse Api::echo(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    std::string srequest = request.encode();
    //std::osyncstream(std::cout) << srequest << std::endl;
    auto headers = request.headers;
    auto body = request.body;
    return response(request, 200, std::move(headers), std::move(body));
    HttpResponse response(
        200,
        HttpHeaders(),
        request.body,
        request.headers
    );
    return response;
}

util::web::http::HttpResponse Api::hello(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn) {
    std::string srequest = request.encode();
    //std::osyncstream(std::cout) << srequest << std::endl;
    return response(request, 200, HttpHeaders(), "<html><body><h1>Hello</h1></body></html>");
}

void Api::onInit() {
    // populating cache
    auto [err1, vusers] = db->getUsers();
    auto [err2, vaddrBooks] = db->getAddressBooks();
    auto [err3, vchats] = db->getChats();
    SharedCache::UsersV susers(vusers.begin(), vusers.end());
    SharedCache::AddressBooksV saddrBooks(vaddrBooks.begin(), vaddrBooks.end());
    SharedCache::ChatsV schats(vchats.begin(), vchats.end());
    sharedCache.init(std::move(susers), std::move(saddrBooks), std::move(schats));
}

std::pair<bool, size_t> Api::userIsAuthenticated(const util::web::http::HttpRequest& request) {
    auto cookies = request.headers.cookies();
    std::string userId;
    std::string authToken;
    if (auto iter = cookies.find("userId"); iter != cookies.end()) {
        userId = iter->second;
    }
    if (auto iter = cookies.find("authToken"); iter != cookies.end()) {
        authToken = iter->second;
    }
    if (userId.empty() || authToken.empty()) return { false, 0 };
    return { sharedCache.userIsAuthentificated(std::stoull(userId), authToken), std::stoull(userId) };
}

std::string Api::generateAuthToken(const std::string& username, const std::string& pwdHash) {
    return sha256(std::format("{}:{}:{}", username, pwdHash, tsMcs()));
}