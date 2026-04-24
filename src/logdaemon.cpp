#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
int main() {
    std::cerr << "logdaemon is not supported on Windows in this project configuration.\n";
    return 1;
}
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

struct Outputs {
    bool consoleEnabled {true};
    std::optional<std::ofstream> file;
    std::mutex mutex;
};

std::vector<std::string> splitLines(const std::string& buffer, std::string* remainder) {
    std::vector<std::string> lines;
    std::size_t start = 0;
    while (true) {
        const std::size_t pos = buffer.find('\n', start);
        if (pos == std::string::npos) {
            break;
        }
        if (pos > start) {
            lines.push_back(buffer.substr(start, pos - start));
        }
        start = pos + 1;
    }
    *remainder = buffer.substr(start);
    return lines;
}

void writeLine(Outputs& outputs, const std::string& line) {
    std::lock_guard<std::mutex> lock(outputs.mutex);
    if (outputs.consoleEnabled) {
        std::cout.write(line.data(), static_cast<std::streamsize>(line.size()));
        std::cout.put('\n');
        std::cout.flush();
    }
    if (outputs.file.has_value() && outputs.file->is_open()) {
        outputs.file->write(line.data(), static_cast<std::streamsize>(line.size()));
        outputs.file->put('\n');
        outputs.file->flush();
    }
}

void handleClient(int clientFd, Outputs* outputs, std::atomic<bool>* running) {
    std::string carry;
    std::string buffer;
    buffer.resize(4096);

    while (running->load(std::memory_order_relaxed)) {
        const ssize_t readBytes = read(clientFd, buffer.data(), buffer.size());
        if (readBytes <= 0) {
            break;
        }

        std::string remainder;
        const std::string combined = carry + std::string(buffer.data(), static_cast<std::size_t>(readBytes));
        const std::vector<std::string> lines = splitLines(combined, &remainder);
        carry = remainder;

        for (const std::string& line : lines) {
            if (!line.empty()) {
                writeLine(*outputs, line);
            }
        }
    }

    close(clientFd);
}

int createListeningSocketTcp(const std::string& host, const std::string& port, std::string* boundAddr) {
    struct addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    struct addrinfo* results = nullptr;
    const int gai = getaddrinfo(host.empty() ? nullptr : host.c_str(), port.c_str(), &hints, &results);
    if (gai != 0) {
        return -1;
    }

    int listenFd = -1;
    for (struct addrinfo* rp = results; rp != nullptr; rp = rp->ai_next) {
        const int fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd < 0) {
            continue;
        }

        int reuse = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        if (bind(fd, rp->ai_addr, static_cast<socklen_t>(rp->ai_addrlen)) != 0) {
            close(fd);
            continue;
        }

        if (listen(fd, 64) != 0) {
            close(fd);
            continue;
        }

        sockaddr_storage local {};
        socklen_t localLen = sizeof(local);
        if (getsockname(fd, reinterpret_cast<sockaddr*>(&local), &localLen) == 0) {
            char hostBuf[NI_MAXHOST] {};
            char serviceBuf[NI_MAXSERV] {};
            if (getnameinfo(reinterpret_cast<sockaddr*>(&local),
                            localLen,
                            hostBuf,
                            sizeof(hostBuf),
                            serviceBuf,
                            sizeof(serviceBuf),
                            NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
                *boundAddr = std::string(hostBuf) + ":" + std::string(serviceBuf);
            }
        }

        listenFd = fd;
        break;
    }

    freeaddrinfo(results);
    return listenFd;
}

int createListeningSocketUnix(const std::string& path, std::string* boundAddr) {
    if (path.empty() || path.size() >= sizeof(sockaddr_un::sun_path)) {
        return -1;
    }

    const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }

    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path.c_str(), sizeof(addr.sun_path) - 1);

    unlink(path.c_str());
    if (bind(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close(fd);
        return -1;
    }

    if (listen(fd, 64) != 0) {
        close(fd);
        return -1;
    }

    *boundAddr = "unix:" + path;
    return fd;
}

}  // namespace

int main(int argc, char** argv) {
    std::string listenAddr = "127.0.0.1:7777";
    std::string filePath;
    bool consoleEnabled = true;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--listen" && i + 1 < argc) {
            listenAddr = argv[++i];
        } else if (arg == "--file" && i + 1 < argc) {
            filePath = argv[++i];
        } else if (arg == "--no-console") {
            consoleEnabled = false;
        } else if (arg == "--help") {
            std::cout << "Usage: logdaemon [--listen host:port] [--file path] [--no-console]\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << arg << "\n";
            return 2;
        }
    }

    Outputs outputs;
    outputs.consoleEnabled = consoleEnabled;
    if (!filePath.empty()) {
        outputs.file.emplace(filePath, std::ios::out | std::ios::app);
        if (!outputs.file->is_open()) {
            std::cerr << "Failed to open log file: " << filePath << "\n";
            return 1;
        }
    }

    std::string boundAddr;
    int listenFd = -1;
    if (listenAddr.rfind("unix:", 0) == 0) {
        listenFd = createListeningSocketUnix(listenAddr.substr(std::strlen("unix:")), &boundAddr);
    } else if (!listenAddr.empty() && listenAddr[0] == '/') {
        listenFd = createListeningSocketUnix(listenAddr, &boundAddr);
    } else {
        const std::size_t colon = listenAddr.rfind(':');
        if (colon == std::string::npos) {
            std::cerr << "Invalid --listen value (expected host:port or unix:<path>): " << listenAddr << "\n";
            return 2;
        }
        const std::string host = listenAddr.substr(0, colon);
        const std::string port = listenAddr.substr(colon + 1);
        listenFd = createListeningSocketTcp(host, port, &boundAddr);
    }
    if (listenFd < 0) {
        std::cerr << "Failed to bind/listen on " << listenAddr << "\n";
        return 1;
    }

    if (!boundAddr.empty()) {
        std::cout << "LISTENING " << boundAddr << "\n";
        std::cout.flush();
    }

    std::atomic<bool> running {true};
    std::vector<std::thread> clients;
    clients.reserve(8);

    while (running.load(std::memory_order_relaxed)) {
        sockaddr_storage peer {};
        socklen_t peerLen = sizeof(peer);
        const int clientFd = accept(listenFd, reinterpret_cast<sockaddr*>(&peer), &peerLen);
        if (clientFd < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        clients.emplace_back(handleClient, clientFd, &outputs, &running);
    }

    running.store(false, std::memory_order_relaxed);
    close(listenFd);
    for (std::thread& client : clients) {
        if (client.joinable()) {
            client.join();
        }
    }

    return 0;
}
#endif
