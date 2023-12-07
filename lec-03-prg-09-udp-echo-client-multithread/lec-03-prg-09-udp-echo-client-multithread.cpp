#include <iostream>

#include <chrono>
#include <thread>

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")

using int32 = __int32;
using namespace std;

// 오류를 처리하고 오류 메시지를 출력하는 함수
void HandleError(const char* cause)
{
    int32 error = ::WSAGetLastError();
    cout << cause << " ErrorMessage : " << error << endl;
}

int main()
{
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData))  // Winsock 초기화
        return 0;

    // UDP : SOCK_DGRAM
    SOCKET clientSocket = ::socket(AF_INET, SOCK_DGRAM, 0);  // UDP용 소켓 생성
    if (clientSocket == INVALID_SOCKET)
    {
        HandleError("Socket");  // 소켓 생성 오류 처리
        return 0;
    }

    SOCKADDR_IN serverAddr;
    ::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    ::inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);  // 서버 주소 설정
    serverAddr.sin_port = ::htons(65456);

    // Connect
    ::connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr));  // 서버에 연결

    cout << "> echo-client is activated" << endl;

    // Send
    char sendBuffer[1024];
    while (true)
    {
        cout << "> "; cin >> sendBuffer;  // 사용자로부터 입력 받음

        int32 result = ::sendto(clientSocket, sendBuffer, sizeof(sendBuffer), 0, (SOCKADDR*)&serverAddr, sizeof(serverAddr));

        if (result == SOCKET_ERROR)
        {
            HandleError("SendTo");  // 데이터 전송 오류 처리
            return 0;
        }

        if (!strncmp(sendBuffer, "quit", sizeof("quit")))
            break;

        SOCKADDR_IN recvAddr;
        ::memset(&recvAddr, 0, sizeof(recvAddr));
        int32 addrLen = sizeof(recvAddr);

        char recvBuffer[1024];

        int32 recvLen = ::recvfrom(clientSocket, recvBuffer, sizeof(recvBuffer), 0, (SOCKADDR*)&recvAddr, &addrLen);

        if (recvLen <= 0)
        {
            HandleError("RecvFrom");  // 데이터 수신 오류 처리
            return 0;
        }

        cout << "> received: " << recvBuffer << endl;
    }

    cout << "> echo-client is de-activated" << endl;

    ::closesocket(clientSocket);  // 소켓 닫기

    ::WSACleanup();  // Winsock 정리

    return 0;
}

