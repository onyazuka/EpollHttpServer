#include "ProjLogger.hpp"
#include "TcpServer.hpp"
#include "test/testHttp.hpp"

using namespace std;
using namespace inet::tcp;

int main()
{
    initLogger(LogLevel::debug);
    
    HttpServer::get().registerRoute("/echo", [](const util::web::http::HttpRequest& request) -> util::web::http::HttpResponse {
        std::string srequest = request.encode();
        //std::osyncstream(std::cout) << srequest << std::endl;
        util::web::http::HttpResponse response;
        response.status = 200;
        response.statusText = "OK";
        response.version = "HTTP/1.1";
        response.headers = request.headers;
        response.body = request.body;
        return response;
    });

    std::string serverAddr = "0.0.0.0";
    uint16_t port = 443;
    TcpServer server("0.0.0.0", port, TcpServer::Options(true));

    return 0;
}