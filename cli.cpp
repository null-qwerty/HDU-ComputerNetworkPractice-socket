#include "include/socket_client.hpp"

int main(int argc, char *argv[])
{
    std::string ipaddr = argv[1];
    int port = std::stoi(argv[2]);
    SocketClient client(ipaddr.c_str(), port);
    client.run();

    return 0;
}