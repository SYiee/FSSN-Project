#include <iostream>
#include <cstring>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main() {
    const char* HOST = "127.0.0.1";
    const int PORT = 65456;

    std::cout << "> echo-server is activated" << std::endl;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char buffer[1024];

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(HOST);
    serverAddr.sin_port = htons(PORT);

    try {
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            throw std::runtime_error("bind() failed");
        }

        if (listen(serverSocket, 1) == SOCKET_ERROR) {
            throw std::runtime_error("listen() failed");
        }

        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);
        std::cout << "> client connected by IP address " << inet_ntoa(clientAddr.sin_addr)
            << " with Port number " << ntohs(clientAddr.sin_port) << std::endl;

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            recv(clientSocket, buffer, sizeof(buffer), 0);
            std::cout << "> echoed: " << buffer << std::endl;
            send(clientSocket, buffer, sizeof(buffer), 0);

            if (strcmp(buffer, "quit") == 0)
                break;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "> " << e.what() << " and program terminated" << std::endl;
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();

    std::cout << "> echo-server is de-activated" << std::endl;

    return 0;
}
