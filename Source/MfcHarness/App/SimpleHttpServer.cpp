#include "pch.h"
#include "SimpleHttpServer.h"
#include <sstream>
#include <string_view>

#pragma comment(lib, "Ws2_32.lib")

namespace
{
std::string BuildStatusLine(int status)
{
    switch (status)
    {
    case 200: return "HTTP/1.1 200 OK\r\n";
    case 204: return "HTTP/1.1 204 No Content\r\n";
    case 400: return "HTTP/1.1 400 Bad Request\r\n";
    case 404: return "HTTP/1.1 404 Not Found\r\n";
    case 409: return "HTTP/1.1 409 Conflict\r\n";
    case 504: return "HTTP/1.1 504 Gateway Timeout\r\n";
    default:  return "HTTP/1.1 500 Internal Server Error\r\n";
    }
}

std::string Trim(const std::string& value)
{
    const auto begin = value.find_first_not_of(" \r\n\t");
    if (begin == std::string::npos)
    {
        return std::string();
    }
    const auto end = value.find_last_not_of(" \r\n\t");
    return value.substr(begin, end - begin + 1);
}
}

SimpleHttpServer::SimpleHttpServer()
{
    WSADATA wsaData;
    ::WSAStartup(MAKEWORD(2, 2), &wsaData);
}

SimpleHttpServer::~SimpleHttpServer()
{
    Stop();
    ::WSACleanup();
}

bool SimpleHttpServer::Start(unsigned short port, HttpHandler handler)
{
    if (m_running)
    {
        return false;
    }

    m_handler = std::move(handler);

    m_listenSocket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET)
    {
        return false;
    }

    BOOL opt = TRUE;
    ::setsockopt(m_listenSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));

    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = ::htonl(INADDR_LOOPBACK);
    service.sin_port = ::htons(port);

    if (::bind(m_listenSocket, reinterpret_cast<SOCKADDR*>(&service), sizeof(service)) == SOCKET_ERROR)
    {
        ::closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    if (::listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        ::closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    m_thread = ::CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
    return m_thread != nullptr;
}

void SimpleHttpServer::Stop()
{
    m_running = false;
    if (m_listenSocket != INVALID_SOCKET)
    {
        ::closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }
    if (m_thread)
    {
        ::WaitForSingleObject(m_thread, 2000);
        ::CloseHandle(m_thread);
        m_thread = nullptr;
    }
}

DWORD WINAPI SimpleHttpServer::ThreadProc(LPVOID param)
{
    auto* server = reinterpret_cast<SimpleHttpServer*>(param);
    server->Run();
    return 0;
}

void SimpleHttpServer::Run()
{
    while (m_running)
    {
        SOCKET clientSocket = ::accept(m_listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET)
        {
            if (!m_running)
            {
                break;
            }
            continue;
        }
        HandleClient(clientSocket);
        ::closesocket(clientSocket);
    }
}

void SimpleHttpServer::HandleClient(SOCKET clientSocket)
{
    constexpr size_t bufferSize = 8192;
    std::string requestData;
    requestData.reserve(1024);

    char buffer[bufferSize];
    int received = 0;
    while ((received = ::recv(clientSocket, buffer, static_cast<int>(bufferSize), 0)) > 0)
    {
        requestData.append(buffer, received);
        if (requestData.find("\r\n\r\n") != std::string::npos)
        {
            break;
        }
    }

    if (requestData.empty())
    {
        return;
    }

    const auto headerEnd = requestData.find("\r\n\r\n");
    const std::string headers = requestData.substr(0, headerEnd + 4);

    std::istringstream stream(headers);
    std::string requestLine;
    std::getline(stream, requestLine);
    requestLine = Trim(requestLine);

    std::istringstream lineStream(requestLine);
    std::string method;
    std::string path;
    std::string version;
    lineStream >> method >> path >> version;

    size_t contentLength = 0;
    std::string headerLine;
    while (std::getline(stream, headerLine))
    {
        headerLine = Trim(headerLine);
        if (headerLine.empty())
        {
            continue;
        }
        const auto colonPos = headerLine.find(':');
        if (colonPos == std::string::npos)
        {
            continue;
        }
        const auto name = headerLine.substr(0, colonPos);
        const auto value = Trim(headerLine.substr(colonPos + 1));
        if (_stricmp(name.c_str(), "Content-Length") == 0)
        {
            contentLength = static_cast<size_t>(std::stoul(value));
        }
    }

    std::string body;
    if (contentLength > 0)
    {
        const size_t alreadyRead = requestData.size() - (headerEnd + 4);
        body = requestData.substr(headerEnd + 4);
        while (body.size() < contentLength)
        {
            received = ::recv(clientSocket, buffer, static_cast<int>(bufferSize), 0);
            if (received <= 0)
            {
                break;
            }
            body.append(buffer, received);
        }
        body.resize(contentLength);
    }

    HttpRequest request{ method, path, body };
    HttpResponse response;

    if (m_handler)
    {
        m_handler(request, response);
    }

    if (response.status == 204)
    {
        response.body.clear();
    }

    std::ostringstream responseStream;
    responseStream << BuildStatusLine(response.status);
    responseStream << "Content-Type: " << response.contentType << "\r\n";
    responseStream << "Content-Length: " << response.body.size() << "\r\n";
    responseStream << "Connection: close\r\n\r\n";
    responseStream << response.body;

    const std::string responseText = responseStream.str();
    ::send(clientSocket, responseText.c_str(), static_cast<int>(responseText.size()), 0);
}
