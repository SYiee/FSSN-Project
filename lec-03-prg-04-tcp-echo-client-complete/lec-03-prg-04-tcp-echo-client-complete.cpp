#include <iostream>
#include <cstring>
#include <string>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    const char* HOST = "127.0.0.1";
    const int PORT = 65456;

    std::cout << "> echo-client is activated" << std::endl;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    SOCKET clientSocket;
    sockaddr_in serverAddr;
    char buffer[1024];
    std::string sendMsg;

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(HOST);
    serverAddr.sin_port = htons(PORT);

    try {
        if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            throw std::runtime_error("connect() failed");
        }

        while (true) {
            std::cout << "> ";
            std::getline(std::cin, sendMsg);

            send(clientSocket, sendMsg.c_str(), sendMsg.length(), 0);

            memset(buffer, 0, sizeof(buffer));
            recv(clientSocket, buffer, sizeof(buffer), 0);

            std::cout << "> received: " << buffer << std::endl;

            if (sendMsg == "quit")
                break;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "> " << e.what() << " and program terminated" << std::endl;
    }

    closesocket(clientSocket);
    WSACleanup();

    std::cout << "> echo-client is de-activated" << std::endl;

    return 0;
}
