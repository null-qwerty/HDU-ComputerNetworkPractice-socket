#include <include/socket_server.hpp>

int main(int argc, char* argv[])
{
    int port = std::stoi(argv[1]);
    if (port < 0 || port > 65535) {
        std::cerr << "port out of range! (0 ~ 65535)" << std::endl;
        return -1;
    }
    SocketServer server(port);
    server.run();

    return 0;
}