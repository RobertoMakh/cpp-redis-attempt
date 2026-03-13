#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./client <command> [key] [value]\n";
        return 1;
    }

    std::string msg = "";
    for (int i = 1; i < argc; ++i) {
        msg += argv[i];
        if (i < argc - 1) msg += " ";
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        std::cout << "Connection failed!\n";
        return 1;
    }

    write(fd, msg.c_str(), msg.length());

    char rbuf[1024] = {};
    read(fd, rbuf, sizeof(rbuf) - 1);
    std::cout << "server says: " << rbuf;

    close(fd);
    return 0;
}