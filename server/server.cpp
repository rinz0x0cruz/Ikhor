#include "server.h"
#include<iostream>
#include<sstream>
#include<unistd.h>

namespace{
    const int BUFFER_SIZE = 30720;

    void log(const std::string &message){
        std::cout << message << std::endl;
    }

    void exitWithError(const std::string &errorMessage){
        log("Error: " + errorMessage);
        exit(1);
    }
}

namespace http{
    TcpServer::TcpServer(std::string ip, int port)
    : m_ip(ip), m_port(port), m_socket(-1), m_new_socket(-1), 
      m_socketAddress(), m_socketAddress_len(sizeof(m_socketAddress)),
      m_serverMessage(buildResponse())
    {
        m_socketAddress.sin_family = AF_INET;
        m_socketAddress.sin_port = htons(m_port);
        m_socketAddress.sin_addr.s_addr = inet_addr(m_ip.c_str());

        if(startServer()){
            std::ostringstream ss;
            ss << "Failed to start server with PORT: " << ntohs(m_socketAddress.sin_port);
            log(ss.str());
        }
    }

    TcpServer::~TcpServer(){
        closeServer();
    }

    int TcpServer::startServer(){
        m_socket = socket(AF_INET, SOCK_STREAM, 0); // Use class member m_socket
        if(m_socket < 0){
            exitWithError("Cannot create socket");
            return 1;
        }
        if(bind(m_socket, (sockaddr *)&m_socketAddress, m_socketAddress_len) < 0){
            exitWithError("Cannot connect socket to address");
            return 1;
        }
        return 0;
    }

    void TcpServer::closeServer(){
        if (m_socket >= 0) close(m_socket);  // Close server socket if valid
        if (m_new_socket >= 0) close(m_new_socket); // Close client socket if valid
    }

    void TcpServer::startListen(){
        if(listen(m_socket, 20) < 0) exitWithError("Socket listen failed");

        std::ostringstream ss;
        ss << "\n*** Listening on ADDRESS " << inet_ntoa(m_socketAddress.sin_addr) 
           << " PORT: " << ntohs(m_socketAddress.sin_port) << " ***\n\n";
        log(ss.str());

        int bytesReceived;

        while(true){
            log("====== Waiting for a new connection ======\n\n\n");
            acceptConnection(m_new_socket);  // Pass m_new_socket by reference

            char buffer[BUFFER_SIZE] = {0};
            bytesReceived = read(m_new_socket, buffer, BUFFER_SIZE);

            if(bytesReceived < 0)
                exitWithError("Failed to read bytes from client socket connection");

            log("------ Received Request from client ------\n\n");

            sendResponse();

            close(m_new_socket);  // Close connection to client after handling request
            m_new_socket = -1;    // Reset the socket descriptor
        }
    }

    void TcpServer::acceptConnection(int &new_socket){
        new_socket = accept(m_socket, (sockaddr *)&m_socketAddress, &m_socketAddress_len);
        if(new_socket < 0){
            std::ostringstream ss;
            ss << "Server failed to accept incoming connection from ADDRESS: " 
               << inet_ntoa(m_socketAddress.sin_addr) << "; PORT: " 
               << ntohs(m_socketAddress.sin_port);
            exitWithError(ss.str());
        }
    }

    std::string TcpServer::buildResponse(){
        std::string htmlFile = "<!DOCTYPE html><html lang=\"en\"><body><h1> HOME </h1><p> Hello from your Server :) </p></body></html>";
        std::ostringstream ss;
        ss << "HTTP/1.1 200 OK\nContent-Type: text/html\nContent-Length: " << htmlFile.size() << "\n\n" << htmlFile;
        return ss.str();
    }

    void TcpServer::sendResponse(){
        long bytesSent = write(m_new_socket, m_serverMessage.c_str(), m_serverMessage.size());

        if(bytesSent == m_serverMessage.size())
            log("------- Server Response sent to client -------\n\n");
        else
            log("Error sending response to client");
    }
} // namespace http
