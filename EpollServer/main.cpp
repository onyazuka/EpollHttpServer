#include "ProjLogger.hpp"
#include "TcpServer.hpp"

using namespace std;
using namespace inet::tcp;

int main()
{
    initLogger(LogLevel::debug);
    Log.error("HI LOGGER");

    std::string serverAddr = "0.0.0.0";
    uint16_t port = 8080;
    TcpServer server("0.0.0.0", 8080, TcpServer::Options(true));

    return 0;
}