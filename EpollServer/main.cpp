#include "ProjLogger.hpp"
#include "TcpServer.hpp"
#include "test/testHttp.hpp"

using namespace std;
using namespace inet::tcp;
using namespace util::web::http;

int main()
{
    initLogger(LogLevel::debug);
    
    HttpServer::get().registerRoute("/echo", [](const util::web::http::HttpRequest& request) -> util::web::http::HttpResponse {
        std::string srequest = request.encode();
        //std::osyncstream(std::cout) << srequest << std::endl;
        HttpResponse response(
            200,
            HttpHeaders(),
            request.body,
            request.headers
        );
        return response;
    });

    HttpServer::get().registerRoute("/hello", [](const util::web::http::HttpRequest& request) -> util::web::http::HttpResponse {
        std::string srequest = request.encode();
        //std::osyncstream(std::cout) << srequest << std::endl;
        HttpResponse response(
            200,
            HttpHeaders(),
            "<html><body><h1>Hello</h1></body></html>",
            request.headers
        );
        return response;
        });

    std::string serverAddr = "0.0.0.0";
    uint16_t port = 443;
    TcpServer server("0.0.0.0", port, TcpServer::Options(true));

    return 0;
}