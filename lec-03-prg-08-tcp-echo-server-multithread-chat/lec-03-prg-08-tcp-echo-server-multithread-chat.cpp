
#include <iostream>
#include <winsock2.h>
#include <vector>
#include <process.h>
#include <string>
#include <mutex>
#pragma comment(lib, "ws2_32.lib")

using namespace std;
#define PORT 65456
#define BufferSize 1024
#define Cli_MAX 5

vector<SOCKET> Cli_List;
std::mutex mtx;

void RecvThread(void* p)
{
    SOCKET ClientSocket = (SOCKET)p;

    while (1)
    {
        char RecvData[BufferSize] = {};
        int bytesRead = recv(ClientSocket, RecvData, sizeof(RecvData), 0);
        if (bytesRead <= 0)
            break;

        {
            std::unique_lock<std::mutex> lock(mtx);
            if (strcmp(RecvData, "quit") == 0) {
                auto iter = find(Cli_List.begin(), Cli_List.end(), ClientSocket);
                if (iter != Cli_List.end())
                    Cli_List.erase(iter);
                closesocket(ClientSocket);  // Close the socket for the current client
                break;
            }

            for (int i = 0; i < Cli_List.size(); i++)
                send(Cli_List[i], RecvData, bytesRead, 0);

            cout << "> received ( " << RecvData << " ) and echoed to " << Cli_List.size() << " clients" << endl;
        }
    }
}

void checkActiveThreads() {
    while (true) {
        string msg;
        getline(cin, msg);

        if (msg == "quit") {
            std::unique_lock<std::mutex> lock(mtx);
            if (Cli_List.empty()) {
                cout << "> stop procedure started" << endl;
                cout << "> echo-server is de-activated" << endl;
                break;
            }
            else {
                cout << "> active threads are remained: " << Cli_List.size() << " Threads" << endl;
            }
        }
    }
}

int main()
{
    cout << "> echo-server is activated" << endl;
    std::cout << "> server loop running in thread (main thread): Thread-1" << std::endl;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET ServerSocket;
    ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN ServerSocketAddr = {};
    ServerSocketAddr.sin_family = AF_INET;
    ServerSocketAddr.sin_port = htons(PORT);
    ServerSocketAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(ServerSocket, (SOCKADDR*)&ServerSocketAddr, sizeof(ServerSocketAddr));
    listen(ServerSocket, Cli_MAX);

    SOCKADDR_IN ClientAddr = {};
    int ClientSize = sizeof(ClientAddr);

    // Start the thread to check for active threads
    std::thread checkThread(checkActiveThreads);

    while (true)
    {
        SOCKET ClientSocket = accept(ServerSocket, (SOCKADDR*)&ClientAddr, &(ClientSize));

        {
            std::unique_lock<std::mutex> lock(mtx);
            Cli_List.push_back(ClientSocket);
        }

        cout << "> Client connected by IP address " << inet_ntoa(ClientAddr.sin_addr) << " with Port Number " << ntohs(ClientAddr.sin_port) << endl;
        _beginthread(RecvThread, NULL, (void*)ClientSocket);
    }

    closesocket(ServerSocket);
    WSACleanup();

    // Wait for all client threads to finish
    {
        std::unique_lock<std::mutex> lock(mtx);
        for (auto& clientSocket : Cli_List) {
            closesocket(clientSocket);
        }
    }

    checkThread.join(); // Wait for checkActiveThreads thread to finish

    cout << "> echo-server is de-activated" << endl;

    return 0;
}
