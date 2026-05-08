#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>


int main() {
    // 1. 创建 socket
    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "socket failed\n";
        return 1;
    }

    // 2. 配置服务器地址
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;

    std::strncpy(
        addr.sun_path,
        "/tmp/log.sock",
        sizeof(addr.sun_path) - 1);
    // 3. 连接 logdaemon
    if (connect(
            fd,
            reinterpret_cast<sockaddr*>(&addr),
            sizeof(addr)) != 0) {

        std::cerr << "connect failed: "
                  << std::strerror(errno)
                  << "\n";

        close(fd);
        return 1;
    }
    
    // 4. 持续发送日志
    while (true) {
        const std::string log =
            "[INFO] hello from client process\n";
        const ssize_t sent = send(fd, log.data(), log.size(), 0);
        if (sent < 0) {
            std::cerr << "send failed\n";
            break;
        }
        sleep(1);
    }
    
    // 5. 关闭连接
    close(fd);
    return 0;
}