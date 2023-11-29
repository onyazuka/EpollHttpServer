#include "Socket.hpp"
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace inet {

	class SslSocket : public ISocket {
	public:
		SslSocket(std::shared_ptr<ISocket> psock, SSL* ssl);
		~SslSocket();
		int init() const override;
		std::shared_ptr<ISocket> accept(bool setNonBlock) const override;
		ssize_t read(InputSocketBuffer& buf) const override;
		ssize_t write(OutputSocketBuffer& buf) const override;
		//int shutdown(int flags);
		int close() override;
		inline int fd() const override {
			return psock->fd();
		}
		struct SslCtx {
			SslCtx();
			~SslCtx();
			mutable SSL_CTX* sslCtx = nullptr;
		};
		inline SSL* getSsl() const { return ssl; }
	private:
		static SslCtx ctx;
		std::shared_ptr<ISocket> psock;
		mutable SSL* ssl = nullptr;
		static char errBuf[256];
	};

}