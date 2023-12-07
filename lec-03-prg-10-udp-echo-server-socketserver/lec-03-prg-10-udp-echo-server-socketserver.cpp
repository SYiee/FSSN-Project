#include <iostream>
#include <cstring>
#include <vector>

#include <chrono>
#include <thread>
#include <atomic>

#include <WinSock2.h>
#include <MSWSock.h>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32")

using int32 = __int32;
using namespace std;


char recvBuffer[1024];  // ���ŵ� �����͸� �����ϴ� ����

// ������ ó���ϰ� ���� �޽����� ����ϴ� �Լ�
void HandleError(const char* cause)
{
    int32 error = ::WSAGetLastError();
    cout << cause << " Error : " << error << endl;
}

int main()
{
    WSADATA wsaData;
    if (::WSAStartup(MAKEWORD(2, 2), &wsaData))  // Winsock �ʱ�ȭ
        return 0;

    // UDP : SOCK_DGRAM
    SOCKET serverSocket = ::socket(AF_INET, SOCK_DGRAM, 0);  // UDP�� ���� ����
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
        HandleError("Bind");  // ���Ͽ� �ּ� ���ε�
        return 0;
    }

    cout << "> echo-server is activated" << endl;

    while (true)
    {
        SOCKADDR_IN clientAddress;
        ::memset(&clientAddress, 0, sizeof(clientAddress));
        int32 addrLen = sizeof(clientAddress);

        // UDP : recvfrom()
        int32 recvLen = ::recvfrom(serverSocket, recvBuffer, sizeof(recvBuffer), 0,
            (SOCKADDR*)&clientAddress, &addrLen);

        if (recvLen <= 0)
        {
            HandleError("RecvFrom");  // ������ ����
            return 0;
        }

        // UDP : sendto()
        int32 errorCode = ::sendto(serverSocket, recvBuffer, recvLen, 0,
            (SOCKADDR*)&clientAddress, sizeof(clientAddress));

        if (errorCode == SOCKET_ERROR)
        {
            HandleError("SendTo");  // ������ �۽�
            return 0;
        }

        cout << "> echoed: " << recvBuffer << endl;
    }

    cout << "> echo-server is de-activated" << endl;

    ::closesocket(serverSocket);  // ���� �ݱ�

    ::WSACleanup();  // Winsock ����

    return 0;
}
