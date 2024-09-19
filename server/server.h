#ifndef INCLUDE_HTTP_TCPSERVER_LINUX
#define INCLUDE_HTTP_TCPSERVER_LINUX

#include<stdio.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<stdlib.h>
#include<string>

namespace http
{
    class TcpServer{
        public:
            TcpServer(std::string ip, int port);
            ~TcpServer();
            void startListen();
            
        private:
            std::string m_ip;
            int m_port;
            int m_socket;
            int m_new_socket;
            struct sockaddr_in m_socketAddress;
            unsigned int m_socketAddress_len;
            std::string m_serverMessage;

            int startServer();
            void closeServer();
            void acceptConnection(int &new_socket);
            std::string buildResponse();
            void sendResponse();
    };
}

#endif
