#include <iostream>
#include <cstring>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

class ThreadedTCPRequestHandler {
public:
    ThreadedTCPRequestHandler(SOCKET clientSocket) : clientSocket(clientSocket) {}

    void handle() {
        // Show a client connection information
        std::cout << "> client connected by IP address " << getClientIP() << " with Port number " << getClientPort() << std::endl;

        while (true) {
            char buffer[1024];
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

            if (bytesReceived <= 0) {
                break;
            }

            // Null-terminate the received data
            buffer[bytesReceived] = '\0';

            // Echo
            send(clientSocket, buffer, bytesReceived, 0);

            std::cout << "> echoed: " << buffer << std::endl;
            //std::cout << "> echoed: " << buffer << " by " << std::this_thread::get_id() << std::endl;

            if (strcmp(buffer, "quit") == 0) {
                break;
            }
        }

        closesocket(clientSocket);
    }

private:
    SOCKET clientSocket;

    int getClientPort() const {
        sockaddr_in addr;
        int len = sizeof(addr);
        getpeername(clientSocket, (struct sockaddr*)&addr, &len);
        return ntohs(addr.sin_port);
    }

    std::string getClientIP() {
        sockaddr_in addr;
        int len = sizeof(addr);
        getpeername(clientSocket, (struct sockaddr*)&addr, &len);

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(addr.sin_addr), clientIP, INET_ADDRSTRLEN);

        return std::string(clientIP);
    }

};

class ThreadedTCPServer {
public:
    ThreadedTCPServer(const std::string& host, int port) : host(host), port(port) {}

    void startServer() {
        // Initialize Winsock
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            std::cerr << "WSAStartup failed\n";
            return;
        }

        // Socket creation
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            std::cerr << "Error creating socket\n";
            WSACleanup();
            return;
        }

        // Address configuration
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(port);

        // Binding
        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
            std::cerr << "Error binding socket\n";
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        // Listening
        if (listen(serverSocket, 1) == SOCKET_ERROR) {
            std::cerr << "Error listening on socket\n";
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        std::cout << "> echo-server is activated\n";

        while (true) {
            // Client connection waiting
            SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "Error accepting connection\n";
                closesocket(serverSocket);
                WSACleanup();
                return;
            }

            // Handle client in a separate thread
            std::async(std::launch::async, &ThreadedTCPRequestHandler::handle, ThreadedTCPRequestHandler(clientSocket));
        }
    }

    ~ThreadedTCPServer() {
        closesocket(serverSocket);
        WSACleanup();
    }

private:
    SOCKET serverSocket;
    sockaddr_in serverAddress;
    std::string host;
    int port;
};

int main() {
    const std::string HOST = "localhost";
    const int PORT = 65456;

    ThreadedTCPServer server(HOST, PORT);
    server.startServer();

    return 0;
}

