#include <LocalServer.hpp>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

static std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '%' && i + 2 < s.size()) {
            char hex[3] = { s[i + 1], s[i + 2], '\0' };
            out += static_cast<char>(strtol(hex, nullptr, 16));
            i += 2;
        } else if (s[i] == '+') {
            out += ' ';
        } else {
            out += s[i];
        }
    }
    return out;
}

static std::string getQueryParam(const std::string& query, const std::string& key) {
    size_t pos = 0;
    while (pos < query.size()) {
        const auto eq  = query.find('=', pos);
        if (eq == std::string::npos) break;
        const auto amp = query.find('&', eq + 1);
        if (query.substr(pos, eq - pos) == key) {
            const auto val = query.substr(eq + 1,
                amp == std::string::npos ? std::string::npos : amp - eq - 1);
            return urlDecode(val);
        }
        if (amp == std::string::npos) break;
        pos = amp + 1;
    }
    return "";
}

static const char SUCCESS_PAGE[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Connection: close\r\n\r\n"
    "<!DOCTYPE html><html><head><meta charset='utf-8'>"
    "<meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>Spotify Switch</title>"
    "<style>body{margin:0;display:flex;align-items:center;justify-content:center;"
    "min-height:100vh;background:#121212;color:#fff;font-family:system-ui}"
    "h1{color:#1DB954}.card{text-align:center;padding:40px}</style></head>"
    "<body><div class='card'><h1>Autorizacion completada</h1>"
    "<p>Vuelve a tu Nintendo Switch.</p></div></body></html>\r\n";

static const char ERROR_PAGE[] =
    "HTTP/1.1 400 Bad Request\r\n"
    "Content-Type: text/html; charset=utf-8\r\n"
    "Connection: close\r\n\r\n"
    "<!DOCTYPE html><html><body style='background:#121212;color:#fff;"
    "font-family:system-ui;text-align:center;padding:40px'>"
    "<h1 style='color:#e74c3c'>Error de autorizacion</h1>"
    "<p>Intentalo de nuevo desde la Switch.</p></body></html>\r\n";

LocalServer::LocalServer(int port)
    : port(port), serverFd(-1), threadStarted(false), codeReady(false)
{
    mutexInit(&this->mutex);
}

LocalServer::~LocalServer() {
    stop();
}

bool LocalServer::start() {
    this->serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->serverFd < 0) return false;

    int opt = 1;
    setsockopt(this->serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(static_cast<uint16_t>(this->port));
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(this->serverFd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        close(this->serverFd);
        this->serverFd = -1;
        return false;
    }

    if (listen(this->serverFd, 1) < 0) {
        close(this->serverFd);
        this->serverFd = -1;
        return false;
    }

    const auto rc = threadCreate(&this->thread, LocalServer::threadEntry, this,
                                 nullptr, 0x10000, 0x2C, -2);
    if (R_FAILED(rc)) {
        close(this->serverFd);
        this->serverFd = -1;
        return false;
    }

    threadStart(&this->thread);
    this->threadStarted = true;
    return true;
}

void LocalServer::stop() {
    if (this->serverFd >= 0) {
        shutdown(this->serverFd, SHUT_RDWR);
        close(this->serverFd);
        this->serverFd = -1;
    }
    if (this->threadStarted) {
        threadWaitForExit(&this->thread);
        threadClose(&this->thread);
        this->threadStarted = false;
    }
}

LocalServer::CallbackResult LocalServer::poll() {
    CallbackResult result;
    mutexLock(&this->mutex);
    if (this->codeReady) {
        result.ready = true;
        result.code  = this->capturedCode;
        result.error = this->capturedError;
    }
    mutexUnlock(&this->mutex);
    return result;
}

void LocalServer::threadEntry(void* arg) {
    static_cast<LocalServer*>(arg)->run();
}

void LocalServer::run() {
    while (true) {
        // Wait for connection with 500ms timeout so we can react to stop()
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(this->serverFd, &fds);
        struct timeval tv = { 0, 500000 };
        const int ret = select(this->serverFd + 1, &fds, nullptr, nullptr, &tv);
        if (ret < 0) break; // serverFd was closed by stop()
        if (ret == 0) continue;

        const int client = accept(this->serverFd, nullptr, nullptr);
        if (client < 0) break;

        char buf[4096] = {};
        const auto n = recv(client, buf, sizeof(buf) - 1, 0);

        std::string code, error;
        if (n > 0) {
            const std::string req(buf, static_cast<size_t>(n));
            // Parse: "GET /callback?code=...&state=... HTTP/1.1"
            const auto pathStart = req.find("GET /");
            if (pathStart != std::string::npos) {
                const auto pathEnd = req.find(" HTTP/", pathStart + 4);
                const auto path = req.substr(pathStart + 4,
                    pathEnd == std::string::npos ? std::string::npos : pathEnd - pathStart - 4);
                const auto qPos = path.find('?');
                if (qPos != std::string::npos) {
                    const auto query = path.substr(qPos + 1);
                    code  = getQueryParam(query, "code");
                    error = getQueryParam(query, "error");
                }
            }
        }

        const bool ok = !code.empty();
        send(client, ok ? SUCCESS_PAGE : ERROR_PAGE,
             ok ? sizeof(SUCCESS_PAGE) - 1 : sizeof(ERROR_PAGE) - 1, 0);
        close(client);

        mutexLock(&this->mutex);
        this->capturedCode  = code;
        this->capturedError = error;
        this->codeReady     = true;
        mutexUnlock(&this->mutex);
        break; // one successful callback is enough
    }
}
