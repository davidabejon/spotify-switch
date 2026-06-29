#pragma once
#include <string>

class LocalServer {
public:
    explicit LocalServer(int port = 8080);
    ~LocalServer();

    bool start();
    void stop();

    // Called from the render callback (main thread). Non-blocking: returns
    // immediately if no connection is waiting. On the frame a browser connects,
    // recv blocks briefly (~1-5 ms on local WiFi) then returns.
    // Returns true when a callback URL was received; fills code / error.
    bool tick(std::string& code, std::string& error);

private:
    int port;
    int serverFd;
};
