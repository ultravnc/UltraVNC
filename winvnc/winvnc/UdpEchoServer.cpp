#include "UdpEchoServer.h"
#include <WinSock2.h>
#include <thread>

std::atomic<bool> g_stopServer(false);

void StartEchoServer(int port)
{
    std::thread serverThread(UdpEchoServer, port); // Pass port number as argument
    serverThread.detach(); // Wait for the server thread to finish
}
void UdpEchoServer(int port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return;
    }

    // Create a UDP socket
    SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (serverSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    // Bind the socket to any address and the specified port
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Listen on any interface
    serverAddr.sin_port = htons(port); // Use port 12345
    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        WSACleanup();
        return;
    }

    while (true) {
        // Receive data from client
        char buffer[1024];
        sockaddr_in clientAddr;
        int clientAddrSize = sizeof(clientAddr);
        int bytesReceived = recvfrom(serverSocket, buffer, sizeof(buffer), 0, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (bytesReceived == SOCKET_ERROR) {
            continue; // Continue to next iteration
        }

        // Print received message
        buffer[bytesReceived] = '\0'; // Null-terminate the received data
        // Echo the message back to the client
        if (strcmp(buffer, "&«&«") != 0) {
            if (sendto(serverSocket, buffer, bytesReceived, 0, reinterpret_cast<sockaddr*>(&clientAddr), clientAddrSize) == SOCKET_ERROR) {
                continue; // Continue to next iteration
            }
        }
    }

    // Cleanup
    closesocket(serverSocket);
    WSACleanup();

}