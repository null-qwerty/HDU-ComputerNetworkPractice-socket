# HDU-ComputerNetworkPractice-socket

这是计网实践课程的课程设计，使用 socket 实现简单的 `服务器-客户端` 通信。

# 代码解析

## server

服务器运行过程：
1. 创建 `socket`；
2. 绑定 `socket`；
3. 监听 `socket`；
4. 接受客户端连接；
5. 创建新线程处理客户端消息。

定义 `SocketServer` 类如下：

```cpp
class SocketServer {
private:
    int server_sock;
    struct sockaddr_in server_addr;

    std::map<int, std::thread> client_threads;

public:
    SocketServer(int port); // 构造函数
    ~SocketServer();        // 析构函数
    void cmd();     // 命令输入
    void run();     // 运行
    void sendto(int client_sock, const char *buf, size_t len);  // 向指定客户端发消息
    void broadcast(const char *buf, size_t len);    // 广播消息
};
```

构造函数中初始化 `server_sock`，绑定端口并侦听：

```cpp
SocketServer(int port)
{
    server_sock = socket(PF_INET, SOCK_STREAM, 0);  // 创建 socket
    if (server_sock == -1) {
        std::cerr << "socket() error" << std::endl;
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);    // 本机地址

    if (bind(server_sock, (struct sockaddr *)&server_addr,  // 绑定端口
             sizeof(server_addr)) == -1) {
        std::cerr << "bind() error" << std::endl;
        exit(1);
    }

    if (listen(server_sock, 5) == -1) {   // 监听
        std::cerr << "listen() error" << std::endl;
        exit(1);
    }
}
```

`run` 函数中接受客户端连接，并创建新线程处理客户端消息：

```cpp
void run()
{
    std::thread cmd_thread(&SocketServer::cmd, this);   // 创建命令输入线程
    cmd_thread.detach();    // 分离线程

    while (true) {  // 循环接受客户端连接
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        int client_sock = accept(server_sock,               // 接受连接
                                 (struct sockaddr *)&client_addr,
                                 &client_addr_size);
        if (client_sock == -1) {
            std::cerr << "accept() error" << std::endl;
            exit(1);
        }

        client_threads[client_sock] = std::thread([client_sock, this]() {   // 创建新线程处理客户端消息
            char buf[1024];

            std::cout << "Connected client: " << client_sock << std::endl;
            std::string welcome = "Hello, client! Your ID is " +
                                  std::to_string(client_sock) + ".\n";
            write(client_sock, welcome.c_str(), welcome.length());

            while (1) { // 子线程循环接受客户端消息
                int str_len = read(client_sock, buf, sizeof(buf) - 1);
                if (str_len == 0)
                    break;
                buf[str_len] = 0;
                std::cout << "Message from client " +
                                 std::to_string(client_sock) + ": "
                          << buf << std::endl;
                // write(client_sock, buf, str_len);    // echo
            }
            // 接收到空消息，断开连接
            std::cout << "Disconnected client: " << client_sock
                      << std::endl;
            close(client_sock);
            client_threads.erase(client_sock);
        });
        client_threads[client_sock].detach();   // 分离线程
    }
}
```

`cmd` 函数用于接收命令行输入，实现发送消息和广播功能：

```cpp
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
```

其中，`sendto` 函数用于向指定客户端发送消息，`broadcast` 函数用于广播消息，实现较简单，不再赘述。

## client

客户端运行过程：
1. 创建 `socket`；
2. 连接服务器；
3. 接收命令行输入，发送消息；
4. 接收服务器消息。

定义 `SocketClient` 类如下：

```cpp
class SocketClient {
private:
    int sock;
    struct sockaddr_in addr;

public:
    SocketClient(const char *ip, int port);
    ~SocketClient();
    void send();
    void recv();
    void run();
};
```

构造函数中连接服务器：

```cpp
SocketClient(const char *ip, int port)
{
    sock = socket(PF_INET, SOCK_STREAM, 0);   // 创建 socket
    if (sock == -1) {
        perror("socket() error");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {  // 连接服务器
        perror("connect() error");
        exit(1);
    }
}
```
`run` 函数中创建发送及接收线程：

```cpp
void run()
{
    std::thread send_thread(&SocketClient::send, this);
    std::thread recv_thread(&SocketClient::recv, this);
    send_thread.join();
    recv_thread.join();
}
```

`send` 函数用于发送消息：

```cpp
void send()
{
    char buf[1024];
    while (1) {
        fgets(buf, sizeof(buf), stdin);
        write(sock, buf, strlen(buf));
    }
}
```

`recv` 函数用于接收消息：

```cpp
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
```
# 使用方法

编译：
```bash
g++ srv.cpp -o ./bin/srv
g++ cli.cpp -o ./bin/cli
```

运行服务器：
```bash
./bin/srv PORT
```

运行客户端：
```bash
./bin/cli IP PORT
```

# 一些问题

1. 由于本项目是课程设计，所以代码实现较为简单，没有考虑多线程下的线程安全问题，如多线程对 `std::cout` 的访问。
2. 若连接过程中服务器退出，会导致 fd 无法正常关闭，下次运行 `srv` 会报错 `bind() error`，需要等待一段时间后再次运行。