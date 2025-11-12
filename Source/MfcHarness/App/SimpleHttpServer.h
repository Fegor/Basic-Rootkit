#pragma once

#include <string>
#include <functional>
#include <winsock2.h>
#include <ws2tcpip.h>

struct HttpRequest
{
    std::string method;
    std::string path;
    std::string body;
};

struct HttpResponse
{
    int status = 200;
    std::string contentType = "application/json";
    std::string body;
};

using HttpHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

class SimpleHttpServer
{
public:
    SimpleHttpServer();
    ~SimpleHttpServer();

    bool Start(unsigned short port, HttpHandler handler);
    void Stop();

private:
    SOCKET m_listenSocket = INVALID_SOCKET;
    HANDLE m_thread = nullptr;
    bool m_running = false;
    HttpHandler m_handler;

    static DWORD WINAPI ThreadProc(LPVOID param);
    void Run();
    void HandleClient(SOCKET clientSocket);
};
