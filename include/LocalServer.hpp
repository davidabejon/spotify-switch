#pragma once
#include <string>
#include <switch.h>

class LocalServer {
public:
    struct CallbackResult {
        std::string code;
        std::string error;
        bool ready = false;
    };

    explicit LocalServer(int port = 8080);
    ~LocalServer();

    bool start();
    void stop();
    CallbackResult poll(); // non-blocking

private:
    int port;
    int serverFd;
    Thread thread;
    bool threadStarted;
    Mutex mutex;
    bool codeReady;
    std::string capturedCode;
    std::string capturedError;

    static void threadEntry(void* arg);
    void run();
};
