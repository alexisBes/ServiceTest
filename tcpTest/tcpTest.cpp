// tcpTest.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WINSOCK_DEPRECATED_NO_WARNINGS 
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
typedef int socklen_t;
#pragma comment(lib,"ws2_32.lib")

#include <iostream>
#include <thread>

SOCKADDR_IN sinSer;
SOCKET serverSock;
socklen_t recsize;
bool isGenial = false;
void acceptCLient();

int main()
{
    WSADATA wsadata;
    int iresult;
    iresult = WSAStartup(MAKEWORD(2, 2), &wsadata);
    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    sinSer.sin_addr.s_addr = inet_addr("127.0.0.1"); // give automaticly a IP adress
    sinSer.sin_family = AF_INET;//for IPv6 use AF_INET6
    sinSer.sin_port = htons(5000);
    if (serverSock == INVALID_SOCKET)
    {
        throw "Socket invalide !!!";
    }
    recsize = sizeof(sinSer);
    bind(serverSock, (SOCKADDR*)&sinSer, recsize);

    listen(serverSock, 5);

    std::thread test(acceptCLient);
    test.detach();

    Sleep(1000);


    shutdown(serverSock, 2);
    closesocket(serverSock);
    std::cout << "Hello World!\n";
    while (!isGenial)
    {}
    std::cout << "Ca fonctionne!\n";
    WSACleanup();
}

void acceptCLient()
{
    try
    {
        SOCKADDR_IN csin;
        socklen_t crecsize = sizeof(csin);
        std::cout << "J'accepte les critiques!\n";
        SOCKET socket = accept(serverSock, (SOCKADDR*)&csin, &crecsize);
        if (socket == SOCKET_ERROR)
        {
            std::cout << "J'ai cassé!\n";
        }
    }
    catch (const std::exception&)
    {
        std::cout << "J'ai cassé!\n";
    }
    isGenial = true;
}
