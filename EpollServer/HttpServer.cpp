#include "HttpServer.hpp"

using namespace util::web::http;

HttpServer::HttpServer() {

}

HttpServer& HttpServer::get() {
	static HttpServer srv{};
	return srv;
}

void HttpServer::registerRoute(const std::string& route, RouteHandlerT handler) {
	if (_routes.find(route) != _routes.end()) {
		throw std::logic_error("route already exists");
	}
	_routes[route] = handler;
}

void HttpServer::unregisterRoute(const std::string& route) {
	_routes.erase(route);
}

util::web::http::HttpResponse HttpServer::callRoute(const std::string& route, const util::web::http::HttpRequest& request) const {
	if (auto iter = _routes.find(route); iter != _routes.end()) {
		return iter->second(request);
	}
	return defaultReponse(404, request);
}

util::web::http::HttpResponse HttpServer::defaultReponse(size_t statusCode, const util::web::http::HttpRequest& request) const {
	HttpHeaders headers;
	util::web::http::HttpResponse response{
		statusCode,
		HttpHeaders()
	};
	return response;
}