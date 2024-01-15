#include "MessengerDb.hpp"
#include "HttpServer.hpp"
#include "SharedCache.hpp"
#include "Http.hpp"
#include "crypto.hpp"
#include "Json.hpp"

class Api {
public:
	Api(std::unique_ptr<db::MessengerDb> pdb);

	/*
		OPTIONS response for CORS request.
	*/
	util::web::http::HttpResponse onOptions(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	//inline HttpServer& http() const { return HttpServer::get(); }
	/*
		POST /user/login
		input:
			json:
				{
					"username": "...",
					"pwdHash": "..."
				}
		output:
			cookies 'userId' and 'authToken'
	*/
	util::web::http::HttpResponse userRegisterOrLogin(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		POST /user/logout
		input:
			empty (cookies)
		output:
			unsets cookies 'userId' and 'authToken'
	*/
	util::web::http::HttpResponse userLogout(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		GET /user/find* (/user/find?username=neko)
		input:
			json:
				{
					"username": "..."
				}
		output:
			userId - number
	*/
	util::web::http::HttpResponse userFind(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		POST /contact
		input:
			json:
				{
					"withId": N
				}
		output: 
			json:
				{
					{addrBook}
				}
	*/
	util::web::http::HttpResponse contactAdd(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		GET /contact
		input:
			empty(cookies)
		output:
			{
				contacts: {id:...,},
				users: {id1: username1, ,,,},
			}
	*/
	util::web::http::HttpResponse contactsGetForId(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		DELETE /contact
		input:
			json:
				{
					"id": N
				}
		output:
			empty
	*/
	util::web::http::HttpResponse contactDelete(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		POST /chat
		input:
			json:
				{
					"withId": N
				}
		output:
			json:
				{
					{chat}
				}
	*/
	util::web::http::HttpResponse chatAdd(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		GET /chat
		input:
			empty(cookies)
		output:
			{
				chats: {id:...,},
				users: {id1: username1, ,,,},
			}
	*/
	util::web::http::HttpResponse chatsGetForId(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		DELETE /chat
		input:
			json:
				{
					"id": N
				}
		output:
			empty
	*/
	util::web::http::HttpResponse chatDelete(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		POST /message
		input:
			json:
				{
					"chatId": N,
					"message": "..."
				}
		output:
			json txtMessage
	*/
	util::web::http::HttpResponse txtMessageAdd(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	/*
		GET /message?chatId=N
		input:
			empty
		output:
			{
				messages: {id:...,},
				users: {id1: username1, ,,,},
			}
	*/
	util::web::http::HttpResponse txtMessagesGetForChatId(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	util::web::http::HttpResponse storageGet(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	util::web::http::HttpResponse eventsSubscribe(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);

	util::web::http::HttpResponse echo(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);
	util::web::http::HttpResponse hello(const util::web::http::HttpRequest& request, HttpServer::CallbackMsgFn cbMsgFn = nullptr);
private:
	util::web::http::HttpResponse response(const util::web::http::HttpRequest& request, size_t code, util::web::http::HttpHeaders&& headers = {}, std::string&& body = "");
	void onInit();
	// returns is auth flag and user id
	std::pair<bool, size_t> userIsAuthenticated(const util::web::http::HttpRequest& request);
	std::string generateAuthToken(const std::string& username, const std::string& pwdHash);
	std::unique_ptr<db::MessengerDb> db;
	SharedCache sharedCache;
	std::function<std::pair<std::string, util::web::json::Node>(const db::User&)> usernameByIdExtractor;
};