#include "server.h"

int main(){
    using namespace http;
    TcpServer server = TcpServer("127.0.0.1",1234);
    server.startListen();
    
    return 0;
}