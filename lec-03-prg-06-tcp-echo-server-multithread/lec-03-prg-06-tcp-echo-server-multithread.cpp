#include <iostream>
#include <cstring>
#include <thread>
#include <string>
#include <mutex>
#include <vector>
#include <algorithm>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

std::size_t serverThreadIndex = 0;  // 서버 스레드의 인덱스
std::mutex serverThreadIndexMutex;

std::size_t clientThreadCounter = 0;  // 클라이언트 스레드의 인덱스 카운터
std::mutex clientThreadCounterMutex;

std::vector<std::pair<std::thread, std::size_t>> clientThreads;
std::mutex clientThreadsMutex;

class MyTCPSocketHandler {
public:
    MyTCPSocketHandler(SOCKET clientSocket, sockaddr_in clientAddr, std::size_t threadIndex)
        : clientSocket(clientSocket), clientAddr(clientAddr), threadIndex(threadIndex) {}

    void handle() {
        //std::cout << "> client connected by IP address " << inet_ntoa(clientAddr.sin_addr)
        //    << " with Port number " << ntohs(clientAddr.sin_port) << " on Thread-" << threadIndex + 1 << std::endl;
        std::cout << "> client connected by IP address " << inet_ntoa(clientAddr.sin_addr)
            << " with Port number " << ntohs(clientAddr.sin_port)<<  std::endl;

        char buffer[1024];
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

            if (bytesReceived <= 0) {
                std::cerr << "> recv() failed or connection closed" << std::endl;
                break;
            }

            std::cout << "> echoed: " << buffer << " by Thread-" << threadIndex + 1 +1 << std::endl;
            send(clientSocket, buffer, bytesReceived, 0);

            if (strcmp(buffer, "quit") == 0) {
                std::cout << "> client disconnected from Thread-" << threadIndex + 1 +1<< std::endl;
                break;
            }
        }

        closesocket(clientSocket);

        // 클라이언트 스레드 종료 후, 벡터에서 제거
        {
            std::lock_guard<std::mutex> lock(clientThreadsMutex);
            auto it = std::find_if(clientThreads.begin(), clientThreads.end(),
                [this](const auto& pair) { return pair.second == this->threadIndex; });

            if (it != clientThreads.end()) {
                it->first.detach();
                clientThreads.erase(it);
            }
        }
    }

private:
    SOCKET clientSocket;
    sockaddr_in clientAddr;
    std::size_t threadIndex;
};

void handleConnection(SOCKET clientSocket, sockaddr_in clientAddr) {
    std::size_t myThreadIndex;
    {
        std::lock_guard<std::mutex> lock(clientThreadCounterMutex);
        myThreadIndex = clientThreadCounter++;
    }

    MyTCPSocketHandler handler(clientSocket, clientAddr, myThreadIndex);
    handler.handle();
}

void checkActiveThreads() {
    while (true) {
        std::string msg;
        std::getline(std::cin, msg);

        if (msg == "quit") {
            std::lock_guard<std::mutex> lock(clientThreadsMutex);

            if (clientThreads.empty()) {
                std::cout << "> stop procedure started" << std::endl;
                std::cout << "> echo-server is de-activated" << std::endl;
                
                break;
            }
            else {
                std::cout << "> active threads are remained: " << clientThreads.size() << " Threads" << std::endl;
            }
        }
    }
}

int main() {
    const char* HOST = "127.0.0.1";
    const int PORT = 65456;

    std::cout << "> echo-server is activated" << std::endl;
    std::cout << "> server loop running in thread (main thread): Thread-1" << std::endl;

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Failed to initialize Winsock." << std::endl;
        return 1;
    }

    SOCKET serverSocket;
    sockaddr_in serverAddr;

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(HOST);
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "> bind() failed and program terminated" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "> listen() failed and program terminated" << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    std::thread checkThread(checkActiveThreads);

    while (true) {
        SOCKET clientSocket;
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(serverSocket, &readSet);

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100;  // 0.1 second

        int ready = select(0, &readSet, nullptr, nullptr, &timeout);

        if (ready == SOCKET_ERROR) {
            std::cerr << "> select() failed" << std::endl;
            break;
        }

        if (ready > 0 && FD_ISSET(serverSocket, &readSet)) {
            clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrLen);

            if (clientSocket == INVALID_SOCKET) {
                std::cerr << "> accept() failed" << std::endl;
                continue;
            }

            std::lock_guard<std::mutex> lock(clientThreadsMutex);
            clientThreads.emplace_back(std::thread(handleConnection, clientSocket, clientAddr), clientThreadCounter);
        }
    }

    closesocket(serverSocket);
    WSACleanup();

    // 모든 클라이언트 스레드가 종료될 때까지 대기
    {
        std::lock_guard<std::mutex> lock(clientThreadsMutex);
        auto it = clientThreads.begin();
        while (it != clientThreads.end()) {
            it->first.join();
            it = clientThreads.erase(it);
        }
    }

    checkThread.join(); // checkActiveThreads 스레드 종료 기다림

    std::cout << "> echo-server is de-activated" << std::endl;

    return 0;
}