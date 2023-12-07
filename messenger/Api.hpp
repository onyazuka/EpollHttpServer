#include "MessengerDb.hpp"
#include "HttpServer.hpp"
#include "SharedCache.hpp"
#include "Http.hpp"
#include "crypto.hpp"
#include "Json.hpp"

class Api {
public:
	Api(std::unique_ptr<db::MessengerDb> pdb);
	//inline HttpServer& http() const { return HttpServer::get(); }
	util::web::http::HttpResponse userRegisterOrLogin(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse userLogout(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse contactAdd(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse contactsGetForId(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse contactDelete(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse chatAdd(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse chatsGetForId(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse chatDelete(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse txtMessageAdd(const util::web::http::HttpRequest& request);
	util::web::http::HttpResponse txtMessagesGetForChatId(const util::web::http::HttpRequest& request);

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