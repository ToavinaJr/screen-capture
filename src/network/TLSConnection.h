#ifndef TLS_CONNECTION_H
#define TLS_CONNECTION_H

#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>

class TLSConnection {
public:
    TLSConnection();
    ~TLSConnection();

    bool init(const std::string& certFile, const std::string& keyFile);
    bool connect(const std::string& hostname, int port);
    void disconnect();
    bool send(const std::string& data);
    std::string receive();

private:
    SSL_CTX* ctx;
    SSL* ssl;
    int socket;

    void cleanup();
};

#endif // TLS_CONNECTION_H