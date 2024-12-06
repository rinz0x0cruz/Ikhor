#include "server.h"

TcpServer::TcpServer() {
    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        std::cerr << "Socket creation failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Socket creation failed");
    }

    // Set server address details
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Bind failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Bind failed");
    }

    // Start listening for connections
    if (listen(server_sock, MAX_CONNECTIONS) < 0) {
        std::cerr << "Listen failed: " << strerror(errno) << std::endl;
        throw std::runtime_error("Listen failed");
    }
}

TcpServer::~TcpServer() {
    close(server_sock);
}

void TcpServer::start() {
    std::cout << "Server listening on port " << PORT << "..." << std::endl;

    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_sock;

    // Accept and handle client connections
    while ((client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len)) >= 0) {
        std::cout << "Client connected from IP: " << inet_ntoa(client_addr.sin_addr) << std::endl;

        // Set socket timeout for each client
        set_socket_timeout(client_sock, 10);

        // Create a new thread for each client connection
        std::thread client_thread(handle_client, client_sock, client_addr);
        client_thread.detach();
    }
}

void TcpServer::set_socket_timeout(int sock, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;  // Set the timeout duration to 10 seconds
    timeout.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        std::cerr << "Error setting socket timeout: " << strerror(errno) << std::endl;
    }
}

void TcpServer::handle_client(int client_sock, sockaddr_in client_addr) {
    char buffer[512];

    while (true) {
        std::cout << "Enter command to send to agent: ";
        std::string command;
        std::getline(std::cin, command);

        if (command == "exit") {
            std::cout << "Exiting communication with agent." << std::endl;
            break;
        }

        try {
            send(client_sock, command.c_str(), command.length(), 0);
        } catch (const std::exception& e) {
            std::cerr << "Error sending command: " << e.what() << std::endl;
            break;
        }

        ssize_t bytes_received;
        try {
            bytes_received = recv(client_sock, buffer, sizeof(buffer), 0);
            if (bytes_received <= 0) {
                if (bytes_received == 0) {
                    std::cerr << "Connection closed by the agent." << std::endl;
                } else {
                    std::cerr << "Error receiving message or timeout occurred." << std::endl;
                }
                break;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error receiving message: " << e.what() << std::endl;
            break;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the response
        std::cout << "Agent's response: " << buffer << std::endl;
    }

    close(client_sock);
}
