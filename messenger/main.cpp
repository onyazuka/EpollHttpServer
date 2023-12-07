#include "ProjLogger.hpp"
#include "TcpServer.hpp"
#include "test/testHttp.hpp"
#include "MessengerDb.hpp"
#include "Api.hpp"

using namespace std;
using namespace inet::tcp;
using namespace util::web::http;

static constexpr char CertPath[] = "/opt/chat/tls.crt";
static constexpr char KeyPath[] = "/opt/chat/tls.key";

inet::SslTcpNonblockingSocket::SslCtx inet::SslTcpNonblockingSocket::ctx(CertPath, KeyPath);

int main()
{
    initLogger(LogLevel::debug);
    
    auto pdb = std::make_unique<db::MessengerDb>("tcp://127.0.0.1:3306", "onyazuka", "5051", "messenger");
    Api api(std::move(pdb));

    std::string serverAddr = "0.0.0.0";
    uint16_t port = 443;
    TcpServer server("0.0.0.0", port, TcpServer::Options(true));

    return 0;
}