#pragma once

#include <cstdlib>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <net/if.h>
#include <thread>
#include <iostream>

class SocketClient {
private:
    int sock;
    struct sockaddr_in addr;

public:
    SocketClient(const char *ip, int port)
    {
        sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock == -1) {
            perror("socket() error");
            exit(1);
        }

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip);

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
            perror("connect() error");
            exit(1);
        }
    }

    ~SocketClient()
    {
        close(sock);
    }

    void send()
    {
        char buf[1024];
        while (1) {
            fgets(buf, sizeof(buf), stdin);
            write(sock, buf, strlen(buf));
        }
    }
    void recv()
    {
        char buf[1024];
        while (1) {
            int str_len = read(sock, buf, sizeof(buf) - 1);
            buf[str_len] = 0;
            if (str_len == 0) {
                std::cout << "Server closed. Exit." << std::endl;
                close(sock);
                exit(-1);
            }
            printf("Message from server: %s", buf);
        }
    }

    void run()
    {
        std::thread send_thread(&SocketClient::send, this);
        std::thread recv_thread(&SocketClient::recv, this);
        send_thread.join();
        recv_thread.join();
    }
};