#include "server.h"

int main() {
    try {
        // Create an instance of TcpServer and start it
        TcpServer server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server initialization failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
