#include "TLSConnection.h"
#include "../utils/Logger.h"
#include "common.h"
#include <cstring>

#ifdef PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
#endif

TLSConnection::TLSConnection() : ctx(nullptr), ssl(nullptr), socket(-1) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    Logger::log(Logger::LogLevel::INFO, "TLSConnection created");
}

TLSConnection::~TLSConnection() {
    disconnect();
    cleanup();
    Logger::log(Logger::LogLevel::INFO, "TLSConnection destroyed");
}

bool TLSConnection::init(const std::string& certFile, const std::string& keyFile) {
    const SSL_METHOD* method = TLS_client_method();
    ctx = SSL_CTX_new(method);
    
    if (!ctx) {
        Logger::log(Logger::LogLevel::WARN, "Failed to create SSL context");
        ERR_print_errors_fp(stderr);
        return false;
    }

    // Load certificates if provided (for server mode)
    if (!certFile.empty() && !keyFile.empty()) {
        if (SSL_CTX_use_certificate_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
            Logger::log(Logger::LogLevel::WARN, "Failed to load certificate file");
            ERR_print_errors_fp(stderr);
            return false;
        }

        if (SSL_CTX_use_PrivateKey_file(ctx, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
            Logger::log(Logger::LogLevel::WARN, "Failed to load private key file");
            ERR_print_errors_fp(stderr);
            return false;
        }

        if (!SSL_CTX_check_private_key(ctx)) {
            Logger::log(Logger::LogLevel::WARN, "Private key does not match certificate");
            return false;
        }
    }

    Logger::log(Logger::LogLevel::INFO, "TLS context initialized");
    return true;
}

bool TLSConnection::connect(const std::string& hostname, int port) {
    // Create socket
    socket = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket < 0) {
        Logger::log(Logger::LogLevel::WARN, "Failed to create socket");
        return false;
    }

    // Resolve hostname
    struct hostent* host = gethostbyname(hostname.c_str());
    if (!host) {
        Logger::log(Logger::LogLevel::WARN, "Failed to resolve hostname");
#ifdef PLATFORM_WINDOWS
        closesocket(socket);
#else
        close(socket);
#endif
        socket = -1;
        return false;
    }

    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, host->h_addr, host->h_length);

    if (::connect(socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::string msg = "Failed to connect to " + hostname + ":" + std::to_string(port);
        Logger::log(Logger::LogLevel::WARN, msg);
#ifdef PLATFORM_WINDOWS
        closesocket(socket);
#else
        close(socket);
#endif
        socket = -1;
        return false;
    }

    // Create SSL connection
    ssl = SSL_new(ctx);
    if (!ssl) {
        Logger::log(Logger::LogLevel::WARN, "Failed to create SSL structure");
        return false;
    }

    SSL_set_fd(ssl, socket);

    // Perform SSL handshake
    if (SSL_connect(ssl) <= 0) {
        Logger::log(Logger::LogLevel::WARN, "SSL handshake failed");
        ERR_print_errors_fp(stderr);
        return false;
    }

    std::string msg = "TLS connection established to " + hostname + ":" + std::to_string(port);
    Logger::log(Logger::LogLevel::INFO, msg);
    return true;
}

void TLSConnection::disconnect() {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }

    if (socket >= 0) {
#ifdef PLATFORM_WINDOWS
        closesocket(socket);
#else
        close(socket);
#endif
        socket = -1;
    }

    Logger::log(Logger::LogLevel::INFO, "TLS connection closed");
}

bool TLSConnection::send(const std::string& data) {
    if (!ssl) {
        Logger::log(Logger::LogLevel::WARN, "Cannot send: SSL not connected");
        return false;
    }

    int bytes_sent = SSL_write(ssl, data.c_str(), data.length());
    if (bytes_sent <= 0) {
        Logger::log(Logger::LogLevel::WARN, "SSL write failed");
        ERR_print_errors_fp(stderr);
        return false;
    }

    return true;
}

std::string TLSConnection::receive() {
    if (!ssl) {
        Logger::log(Logger::LogLevel::WARN, "Cannot receive: SSL not connected");
        return "";
    }

    char buffer[4096];
    int bytes_received = SSL_read(ssl, buffer, sizeof(buffer) - 1);
    
    if (bytes_received <= 0) {
        int error = SSL_get_error(ssl, bytes_received);
        if (error != SSL_ERROR_WANT_READ && error != SSL_ERROR_WANT_WRITE) {
            Logger::log(Logger::LogLevel::WARN, "SSL read failed");
            ERR_print_errors_fp(stderr);
        }
        return "";
    }

    buffer[bytes_received] = '\0';
    return std::string(buffer, bytes_received);
}

void TLSConnection::cleanup() {
    if (ctx) {
        SSL_CTX_free(ctx);
        ctx = nullptr;
    }
}
