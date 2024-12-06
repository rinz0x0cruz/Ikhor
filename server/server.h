#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <string>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <sys/socket.h>
#include <stdexcept>

#define PORT 12345
#define MAX_CONNECTIONS 10

class TcpServer {
public:
    TcpServer();
    ~TcpServer();
    void start();
    void set_socket_timeout(int sock, int seconds);
    static void handle_client(int client_sock, sockaddr_in client_addr);

private:
    int server_sock;
    sockaddr_in server_addr;
};

#endif // SERVER_H
