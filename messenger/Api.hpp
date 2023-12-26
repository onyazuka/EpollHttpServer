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
	util::web::http::HttpResponse onOptions(const util::web::http::HttpRequest& request);

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
	util::web::http::HttpResponse userRegisterOrLogin(const util::web::http::HttpRequest& request);

	/*
		POST /user/logout
		input:
			empty (cookies)
		output:
			unsets cookies 'userId' and 'authToken'
	*/
	util::web::http::HttpResponse userLogout(const util::web::http::HttpRequest& request);

	/*
		GET /user
		input:
			json:
				{
					"username": "..."
				}
		output:
			unsets cookies 'userId' and 'authToken'
	*/
	util::web::http::HttpResponse userFind(const util::web::http::HttpRequest& request);

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
	util::web::http::HttpResponse contactAdd(const util::web::http::HttpRequest& request);

	/*
		GET /contact
		input:
			empty(cookies)
		output:
			json array of address books
	*/
	util::web::http::HttpResponse contactsGetForId(const util::web::http::HttpRequest& request);

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
	util::web::http::HttpResponse contactDelete(const util::web::http::HttpRequest& request);

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
	util::web::http::HttpResponse chatAdd(const util::web::http::HttpRequest& request);

	/*
		GET /chat
		input:
			empty(cookies)
		output:
			json array of chats
	*/
	util::web::http::HttpResponse chatsGetForId(const util::web::http::HttpRequest& request);

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
	util::web::http::HttpResponse chatDelete(const util::web::http::HttpRequest& request);

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
	util::web::http::HttpResponse txtMessageAdd(const util::web::http::HttpRequest& request);

	/*
		GET /message
		input:
			json:
				{
					"chatId": N
				}
		output:
			json array of messages
	*/
	util::web::http::HttpResponse txtMessagesGetForChatId(const util::web::http::HttpRequest& request);

	util::web::http::HttpResponse storageGet(const util::web::http::HttpRequest& request);

	util::web::http::HttpResponse echo(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse hello(const util::web::http::HttpRequest& request);
private:
	util::web::http::HttpResponse response(const util::web::http::HttpRequest& request, size_t code, util::web::http::HttpHeaders&& headers = {}, std::string&& body = "");
	void onInit();
	// returns is auth flag and user id
	std::pair<bool, size_t> userIsAuthenticated(const util::web::http::HttpRequest& request);
	std::string generateAuthToken(const std::string& username, const std::string& pwdHash);
	std::unique_ptr<db::MessengerDb> db;
	SharedCache sharedCache;
};