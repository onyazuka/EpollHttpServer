#include "Api.hpp"
#include <format>
#include "ProjLogger.hpp"
#include "profiling.hpp"

using namespace util::web::http;
using namespace util::web::json;
using namespace util::crypto;
using namespace util::prof;
using namespace db;

#define NotAuthGuard size_t userId = 0; bool auth = false; if (std::tie(auth, userId) = userIsAuthenticated(request); !auth) return response(request, 403);

Api::Api(std::unique_ptr<db::MessengerDb> pdb)
    : db{std::move(pdb)}
{
    onInit();
    
    // test - TODO delete
    HttpServer::get().registerRoute("/echo", Method::GET, [this](const util::web::http::HttpRequest& request) { return echo(request); });
    HttpServer::get().registerRoute("/hello", Method::GET, [this](const util::web::http::HttpRequest& request) { return hello(request); });
    
    HttpServer::get().registerRoute("/user/login", Method::POST, [this](const util::web::http::HttpRequest& request) { return userRegisterOrLogin(request); });
    HttpServer::get().registerRoute("/user/logout", Method::POST, [this](const util::web::http::HttpRequest& request) { return userLogout(request); });
}

util::web::http::HttpResponse Api::userRegisterOrLogin(const util::web::http::HttpRequest& request) {
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
        headers.add("SET-COOKIE", std::format("userId={}\nSET-COOKIE:authToken={}", userId, authToken));
        return response(request, 200, std::move(headers));
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::response(const util::web::http::HttpRequest& request, size_t code, util::web::http::HttpHeaders&& headers, std::string&& body) {
     return HttpResponse(
        code,
        std::move(headers),
        std::move(body),
        request.headers
    );
}

util::web::http::HttpResponse Api::userLogout(const util::web::http::HttpRequest& request) {
    //if (!isUserAuthenticated(request)) return notAuth(request);
    NotAuthGuard;
    HttpHeaders headers;
    headers.add("SET-COOKIE", std::format("userId=deleted; expires=Thu, 01 Jan 1970 00:00:00 GMT\nSET-COOKIE:authToken=deleted; expires=Thu, 01 Jan 1970 00:00:00 GMT"));
    return response(request, 200, std::move(headers));
}

util::web::http::HttpResponse Api::contactAdd(const util::web::http::HttpRequest& request) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto withId = (size_t)json.as<int64_t>("withId");
        auto [err, optAddrBookEntryId] = db->addContactToAddressBook(userId, withId);
        if (err != db::MessengerDb::Error::Ok) {
            throw std::invalid_argument(std::format("can't add contact for id {}: err = {}", userId, (int)err));
        }
        sharedCache.contactAdd({ optAddrBookEntryId.value(), userId, withId });
        return response(request, 200);
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::contactsGetForId(const util::web::http::HttpRequest& request) {
    NotAuthGuard;
    auto contacts = sharedCache.contactGetForId(userId);
    ArrNode resArr;
    for (auto& contact : contacts) {
        resArr.cont().push_back(contact.toObjNode());
        //resArr.cont().push_back()
    }
    HttpHeaders headers;
    headers.add("CONTENT-TYPE", "application/json");
    return HttpResponse(200, std::move(headers), JsonEncoder().encode(Node(resArr)));
}

util::web::http::HttpResponse Api::contactDelete(const util::web::http::HttpRequest& request) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto contactId = (size_t)json.as<int64_t>("id");
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

util::web::http::HttpResponse Api::chatAdd(const util::web::http::HttpRequest& request) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto withId = (size_t)json.as<int64_t>("withId");
        auto [err, optChatEntryId] = db->addChat(userId, withId);
        if (err != db::MessengerDb::Error::Ok) {
            throw std::invalid_argument(std::format("can't add chat for id {}: err = {}", userId, (int)err));
        }
        sharedCache.chatAdd({ optChatEntryId.value(), userId, withId });
        return response(request, 200);
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::chatsGetForId(const util::web::http::HttpRequest& request) {
    NotAuthGuard;
    auto chats = sharedCache.chatsGetForId(userId);
    ArrNode resArr;
    for (auto& chat : chats) {
        resArr.cont().push_back(chat.toObjNode());
        //resArr.cont().push_back()
    }
    HttpHeaders headers;
    headers.add("CONTENT-TYPE", "application/json");
    return HttpResponse(200, std::move(headers), JsonEncoder().encode(Node(resArr)));
}

util::web::http::HttpResponse Api::chatDelete(const util::web::http::HttpRequest& request) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto chatId = (size_t)json.as<int64_t>("id");
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

util::web::http::HttpResponse Api::txtMessageAdd(const util::web::http::HttpRequest& request) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto chatId = (size_t)json.as<int64_t>("chatId");
        auto whoId = (size_t)json.as<int64_t>("whoId");
        std::string message = json.as<std::string>("message");
        auto [err, optId] = db->addTxtMessage(chatId, whoId, message);
        if (err != db::MessengerDb::Error::Ok) {
            return response(request, 400);
        }
        return response(request, 200);
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::txtMessagesGetForChatId(const util::web::http::HttpRequest& request) {
    try {
        NotAuthGuard;
        auto json = JsonDecoder().decode(request.body);
        auto chatId = (size_t)json.as<int64_t>("id");
        auto [err, messages] = db->getTxtMessagesForChat(chatId);
        if (err != db::MessengerDb::Error::Ok) {
            return response(request, 400);
        }
        ArrNode resArr;
        for (auto& msg : messages) {
            resArr.cont().push_back(msg.toObjNode());
            //resArr.cont().push_back()
        }
        HttpHeaders headers;
        headers.add("CONTENT-TYPE", "application/json");
        return HttpResponse(200, std::move(headers), JsonEncoder().encode(Node(resArr)));
    }
    catch (std::exception& ex) {
        Log.info(ex.what());
        return response(request, 400);
    }
}

util::web::http::HttpResponse Api::echo(const util::web::http::HttpRequest& request) {
    std::string srequest = request.encode();
    //std::osyncstream(std::cout) << srequest << std::endl;
    HttpResponse response(
        200,
        HttpHeaders(),
        request.body,
        request.headers
    );
    return response;
}

util::web::http::HttpResponse Api::hello(const util::web::http::HttpRequest& request) {
    std::string srequest = request.encode();
    //std::osyncstream(std::cout) << srequest << std::endl;
    HttpResponse response(
        200,
        HttpHeaders(),
        "<html><body><h1>Hello</h1></body></html>",
        request.headers
    );
    return response;
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
    std::string userId = 0;
    std::string authToken;
    if (auto iter = cookies.find("userId"); iter != cookies.end()) {
        userId = iter->second;
    }
    if (auto iter = cookies.find("authToken"); iter != cookies.end()) {
        authToken = iter->second;
    }
    return { !(userId.empty() || authToken.empty() || !sharedCache.userIsAuthentificated(std::stoull(userId), authToken)), std::stoull(userId) };
}

std::string Api::generateAuthToken(const std::string& username, const std::string& pwdHash) {
    return sha256(std::format("{}:{}:{}", username, pwdHash, tsMcs()));
}