#include "SslSocket.hpp"
#include "ProjLogger.hpp"

using namespace inet;

char SslSocket::errBuf[256];
SslSocket::SslCtx SslSocket::ctx;

SslSocket::SslCtx::SslCtx() {
    static constexpr char CertPath[] = "/opt/chat/crt.crt";
    static constexpr char KeyPath[] = "/opt/chat/key.key";

    const SSL_METHOD* method;

    method = TLS_server_method();

    sslCtx = SSL_CTX_new(method);
    if (!sslCtx) {
        throw std::runtime_error(std::format("Couldn't open ssl context: {}", ERR_error_string(ERR_get_error(), errBuf)));
    }

    if (SSL_CTX_use_certificate_file(sslCtx, CertPath, SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error(std::format("Couldn't use certificate for socket: {}, certificate path is set to {}", ERR_error_string(ERR_get_error(), errBuf), CertPath));
    }

    if (SSL_CTX_use_PrivateKey_file(sslCtx, KeyPath, SSL_FILETYPE_PEM) <= 0) {
        throw std::runtime_error(std::format("Couldn't use private key for socket: {}, private key path is set to {}", ERR_error_string(ERR_get_error(), errBuf), KeyPath));
    }
}

SslSocket::SslCtx::~SslCtx() {
    SSL_CTX_free(sslCtx);
}

SslSocket::SslSocket(std::shared_ptr<ISocket> _psock, SSL* _ssl)
	: psock{ _psock }, ssl{_ssl}
{
	;
}

SslSocket::~SslSocket() {
   
}

int SslSocket::init() const {
    return 0;
}

std::shared_ptr<ISocket> SslSocket::accept(bool setNonBlock) const {
    auto client = psock->accept(setNonBlock);
    if (client == nullptr) {
        return nullptr;
    }
    SSL* clientSsl = nullptr;
    if (clientSsl = SSL_new(ctx.sslCtx); clientSsl == 0) {
        Log.error(std::format("Couldn't create ssl connection for socket {}: {}", psock->fd(), ERR_error_string(ERR_get_error(), errBuf)));
        return nullptr;
    }
    if (SSL_set_fd(clientSsl, client->fd()) == 0) {
        Log.error(std::format("Couldn't set ssl to fd {}: {}", psock->fd(), ERR_error_string(ERR_get_error(), errBuf)));
        return nullptr;
    }
    for (;;) {
        int err = SSL_accept(clientSsl);
        if (err > 0) {
            break;
        }
        else if (SSL_get_error(clientSsl, err) == SSL_ERROR_WANT_READ) {
            continue;
        }
        else if (err <= 0) {
            Log.error(std::format("Couldn't accept ssl to fd {}: {}", psock->fd(), ERR_error_string(SSL_get_error(clientSsl, err), errBuf)));
            return nullptr;
        }
    }
    return std::shared_ptr<ISocket>(new SslSocket(client, clientSsl));
}

ssize_t SslSocket::read(std::span<char> buf) const {
    size_t nbytes = 0;
    for (;;) {
        int n = SSL_read(ssl, buf.data(), buf.size());
        int err = SSL_get_error(ssl, n);
        if (err != SSL_ERROR_NONE) {
            if (err == SSL_ERROR_ZERO_RETURN) {
                Log.debug(std::format("ssl client socket {} has closed connection", psock->fd()));
                return 0;
            }
            else if (err == SSL_ERROR_WANT_READ)
            {
                //Log.debug(std::format("EAGAIN on socket {}", sock.fd()));
                return nbytes ? nbytes : -EAGAIN;
            }
            else {
                // n < 0 - error
                Log.error(std::format("error \"{}\" on socket {}", ERR_error_string(err, errBuf), psock->fd()));
                return -EBADFD;
            }
        }
        else {
            Log.debug(std::format("Read {} bytes from {}", n, psock->fd()));
            nbytes += n;
            //break;
        }
    }
    return nbytes;
}

ssize_t SslSocket::write(std::span<char> buf) const {
    return -1;
}

int SslSocket::close() {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    int res = ::close(psock->fd());
    psock.reset();
    return res;
}
