#include <iostream>
#include <cstring>
#include <vector>
#include <unordered_set>

#include <chrono>
#include <thread>
#include <atomic>

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")

using int32 = __int32;
using namespace std;

char recvBuffer[1024];  // 수신된 데이터를 저장하는 버퍼

// 등록된 클라이언트를 저장하는 집합
struct SocketAddressHash {
    size_t operator()(const SOCKADDR_IN& addr) const {
        return hash<int32>()(addr.sin_addr.s_addr) ^ hash<uint16_t>()(addr.sin_port);
    }
};

struct SocketAddressEqual {
    bool operator()(const SOCKADDR_IN& lhs, const SOCKADDR_IN& rhs) const {
        return lhs.sin_addr.s_addr == rhs.sin_addr.s_addr && lhs.sin_port == rhs.sin_port;
    }
};

unordered_set<SOCKADDR_IN, SocketAddressHash, SocketAddressEqual> registeredClients;

// 오류를 처리하고 오류 메시지를 출력하는 함수
void HandleError(const char* cause)
{
    int32 error = ::WSAGetLastError();
    cout << cause << " Error : " << error << endl;
}

// 클라이언트 등록 함수
void RegisterClient(const SOCKADDR_IN& clientAddress)
{
    registeredClients.insert(clientAddress);
    cout << "> Client registered" << endl;
}

// 클라이언트 등록 해제 함수
void DeregisterClient(const SOCKADDR_IN& clientAddress)
{
    registeredClients.erase(clientAddress);
    cout << "> Client deregistered" << endl;
}

int main()
{
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData))  // Winsock 초기화
        return 0;

    // UDP : SOCK_DGRAM
    SOCKET serverSocket = ::socket(AF_INET, SOCK_DGRAM, 0);  // UDP용 소켓 생성
    if (serverSocket == INVALID_SOCKET)
    {
        HandleError("Socket");
        return 0;
    }

    SOCKADDR_IN serverAddr;
    ::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
    serverAddr.sin_port = ::htons(65456);

    if (::bind(serverSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        HandleError("Bind");
        return 0;
    }

    cout << "> echo-server is activated" << endl;

    while (true)
    {
        SOCKADDR_IN clientAddress;
        ::memset(&clientAddress, 0, sizeof(clientAddress));
        int32 addrLen = sizeof(clientAddress);

        int32 recvLen = ::recvfrom(serverSocket, recvBuffer, sizeof(recvBuffer), 0,
            (SOCKADDR*)&clientAddress, &addrLen);

        if (recvLen <= 0)
        {
            HandleError("RecvFrom");
            return 0;
        }

        // 처리할 특수 명령어를 확인
        if (strncmp(recvBuffer, "#REG", 4) == 0)
        {
            RegisterClient(clientAddress);
            // 클라이언트에게 응답
            int32 errorCode = ::sendto(serverSocket, recvBuffer, recvLen, 0,
                (SOCKADDR*)&clientAddress, sizeof(clientAddress));
            if (errorCode == SOCKET_ERROR)
            {
                HandleError("SendTo");
                return 0;
            }
        }
        else if (strncmp(recvBuffer, "#DREG", 5) == 0)
        {
            DeregisterClient(clientAddress);
            // 클라이언트에게 응답
            int32 errorCode = ::sendto(serverSocket, recvBuffer, recvLen, 0,
                (SOCKADDR*)&clientAddress, sizeof(clientAddress));
            if (errorCode == SOCKET_ERROR)
            {
                HandleError("SendTo");
                return 0;
            }
        }
        else
        {
            // 일반 데이터 처리
            for (const auto& registeredClient : registeredClients)
            {
                // UDP : sendto()
                int32 errorCode = ::sendto(serverSocket, recvBuffer, recvLen, 0,
                    (SOCKADDR*)&registeredClient, sizeof(registeredClient));

                if (errorCode == SOCKET_ERROR)
                {
                    HandleError("SendTo");
                    return 0;
                }
            }

            cout << "> echoed to all clients: " << recvBuffer << endl;
        }
    }

    cout << "> echo-server is de-activated" << endl;

    ::closesocket(serverSocket);

    ::WSACleanup();

    return 0;
}
