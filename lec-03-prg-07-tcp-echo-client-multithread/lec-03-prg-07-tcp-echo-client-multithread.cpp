#include <iostream>
#include <thread>
#include <cstring>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 65456
#define BUFFER_SIZE 1024

void recvHandler(SOCKET clientSocket) {
    char buffer[BUFFER_SIZE];
    while (true) {
        int bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesRead <= 0) {
            std::cerr << "> Error receiving data or server disconnected." << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';  // Null 종결 문자 추가

        std::cout << "> received: " << buffer << std::endl;

        if (strcmp(buffer, "quit") == 0) {
            break;
        }
    }

    closesocket(clientSocket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "> Failed to initialize Winsock." << std::endl;
        return EXIT_FAILURE;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "> Failed to create client socket." << std::endl;
        WSACleanup();
        return EXIT_FAILURE;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddress.sin_port = htons(PORT);

    try {
        if (connect(clientSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) == SOCKET_ERROR) {
            throw std::runtime_error("Connection failed");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "> connect() failed by exception: " << e.what() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    std::cout << "> echo-client is activated" << std::endl;

    std::thread(recvHandler, clientSocket).detach();

    while (true) {
        std::string sendMsg;
        std::cout << "> ";
        std::getline(std::cin, sendMsg);

        send(clientSocket, sendMsg.c_str(), sendMsg.size(), 0);

        if (sendMsg == "quit") {
            break;
        }
    }

    std::cout << "> echo-client is de-activated" << std::endl;
    closesocket(clientSocket);
    WSACleanup();

    return 0;
}


