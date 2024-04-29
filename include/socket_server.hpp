#pragma once

#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <iostream>
#include <map>

class SocketServer {
private:
    int server_sock;
    struct sockaddr_in server_addr;

    std::map<int, std::thread> client_threads;

public:
    SocketServer(int port)
    {
        server_sock = socket(PF_INET, SOCK_STREAM, 0);
        if (server_sock == -1) {
            std::cerr << "socket() error" << std::endl;
            exit(1);
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(server_sock, (struct sockaddr *)&server_addr,
                 sizeof(server_addr)) == -1) {
            std::cerr << "bind() error" << std::endl;
            exit(1);
        }

        if (listen(server_sock, 5) == -1) {
            std::cerr << "listen() error" << std::endl;
            exit(1);
        }
    }

    ~SocketServer()
    {
        close(server_sock);
    }

    void cmd()
    {
        char buf[1024];
        while (1) {
            fgets(buf, sizeof(buf), stdin);
            std::string cmd = buf;

            if (cmd.find("/sendto") == 0) {
                int client_sock = std::stoi(cmd.substr(8));
                sscanf(cmd.c_str(), "/sendto %d", &client_sock);
                fgets(buf, sizeof(buf), stdin);
                sendto(client_sock, buf, strlen(buf));
            } else if (cmd.find("/broadcast") == 0) {
                fgets(buf, sizeof(buf), stdin);
                broadcast(buf, strlen(buf));
            } else if (cmd.find("/exit") == 0) {
                break;
            } else {
                std::cerr << "Invalid command: " << cmd << std::endl;
            }
        }
    }

    void run()
    {
        std::thread cmd_thread(&SocketServer::cmd, this);
        cmd_thread.detach();

        while (true) {
            struct sockaddr_in client_addr;
            socklen_t client_addr_size = sizeof(client_addr);
            int client_sock = accept(server_sock,
                                     (struct sockaddr *)&client_addr,
                                     &client_addr_size);
            if (client_sock == -1) {
                std::cerr << "accept() error" << std::endl;
                exit(1);
            }

            client_threads[client_sock] = std::thread([client_sock, this]() {
                char buf[1024];

                std::cout << "Connected client: " << client_sock << std::endl;
                std::string welcome = "Hello, client! Your ID is " +
                                      std::to_string(client_sock) + ".\n";
                write(client_sock, welcome.c_str(), welcome.length());

                while (1) {
                    int str_len = read(client_sock, buf, sizeof(buf) - 1);
                    if (str_len == 0)
                        break;
                    buf[str_len] = 0;
                    std::cout << "Message from client " +
                                     std::to_string(client_sock) + ": "
                              << buf << std::endl;
                    // write(client_sock, buf, str_len);
                }
                std::cout << "Disconnected client: " << client_sock
                          << std::endl;
                close(client_sock);
                client_threads.erase(client_sock);
            });
            client_threads[client_sock].detach();
        }
    }

    void sendto(int client_sock, const char *buf, size_t len)
    {
        if (client_threads.find(client_sock) == client_threads.end()) {
            std::cerr << "Client not found: " << client_sock << std::endl;
            return;
        }
        std::cout << "Message to client" + std::to_string(client_sock) + ":"
                  << buf << std::endl;
        write(client_sock, buf, len);
        return;
    }

    void broadcast(const char *buf, size_t len)
    {
        std::cout << "Broadcast message: " << buf << std::endl;
        for (auto &pair : client_threads) {
            write(pair.first, buf, len);
        }
    }
};