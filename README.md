# Ikhor
C2 framework

#### server
g++ -o server server.cpp -pthread

#### agent
x86_64-w64-mingw32-g++ -o agent.exe agent.cpp -lws2_32 -static-libgcc -static-libstdc++
