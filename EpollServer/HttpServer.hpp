#pragma once
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include "Http.hpp"

class HttpServer {
public:
	using RouteHandlerT = std::function<util::web::http::HttpResponse(const util::web::http::HttpRequest&)>;
	void registerRoute(const std::string& route, RouteHandlerT handler);
	void unregisterRoute(const std::string& route);
	util::web::http::HttpResponse callRoute(const std::string& route, const util::web::http::HttpRequest& request) const;
	util::web::http::HttpResponse defaultReponse(size_t statusCode) const;
	inline auto& routes() { return _routes; }
	static HttpServer& get();
private:
	HttpServer();
	std::unordered_map<std::string, RouteHandlerT> _routes;
};