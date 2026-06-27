#include <iostream>
#include <string>
#include <thread>
#include <sstream>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <cctype>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "store.h"
#include "wal.h"
#include "parser.h"
#include "thread_pool.h"          // ← NEW

static const int    PORT        = 6380;
static const int    BACKLOG     = 128;
static const size_t CAPACITY    = 1000;
static const size_t NUM_THREADS = std::thread::hardware_concurrency(); // ← NEW

static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

std::string replayCommand(const std::vector<std::string>& tokens, KVStore& store) {
    if (tokens.empty()) return "";
    std::string cmd = toUpper(tokens[0]);
    if (cmd == "SET"    && tokens.size() >= 3) return store.set(tokens[1], tokens[2]);
    if (cmd == "DEL"    && tokens.size() >= 2) return store.del(tokens[1]);
    if (cmd == "EXPIRE" && tokens.size() >= 3)
        return store.expire(tokens[1], std::stoi(tokens[2]));
    return "";
}

std::string handleCommand(const std::vector<std::string>& tokens,
                          KVStore& store, WAL& wal) {
    if (tokens.empty()) return "-ERR empty command\r\n";
    std::string cmd = toUpper(tokens[0]);

    if (cmd == "PING") return store.ping();

    if (cmd == "SET") {
        if (tokens.size() < 3) return "-ERR usage: SET <key> <value>\r\n";
        wal.log("SET " + tokens[1] + " " + tokens[2]);
        return store.set(tokens[1], tokens[2]);
    }
    if (cmd == "GET") {
        if (tokens.size() < 2) return "-ERR usage: GET <key>\r\n";
        auto val = store.get(tokens[1]);
        if (!val) return "$-1\r\n";
        return "$" + std::to_string(val->size()) + "\r\n" + *val + "\r\n";
    }
    if (cmd == "DEL") {
        if (tokens.size() < 2) return "-ERR usage: DEL <key>\r\n";
        wal.log("DEL " + tokens[1]);
        return store.del(tokens[1]);
    }
    if (cmd == "EXPIRE") {
        if (tokens.size() < 3) return "-ERR usage: EXPIRE <key> <seconds>\r\n";
        wal.log("EXPIRE " + tokens[1] + " " + tokens[2]);
        return store.expire(tokens[1], std::stoi(tokens[2]));
    }
    return "-ERR unknown command '" + tokens[0] + "'\r\n";
}

void replayWAL(const std::string& walPath, KVStore& store) {
    std::ifstream f(walPath);
    if (!f.is_open()) return;
    std::string line;
    int count = 0;
    while (std::getline(f, line)) {
        if (line.empty()) continue;
        replayCommand(parseCommand(line), store);
        count++;
    }
    if (count > 0)
        std::cout << "[WAL] Replayed " << count << " commands.\n";
}

void clientHandler(int clientFd, KVStore& store, WAL& wal) {
    char buf[4096];
    while (true) {
        memset(buf, 0, sizeof(buf));
        ssize_t n = recv(clientFd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) break;
        std::string raw(buf);
        while (!raw.empty() && (raw.back() == '\r' || raw.back() == '\n'))
            raw.pop_back();
        auto tokens = parseCommand(raw);
        std::string response = handleCommand(tokens, store, wal);
        send(clientFd, response.c_str(), response.size(), 0);
    }
    close(clientFd);
}

int main() {
    KVStore    store(CAPACITY);
    WAL        wal("wal.log");
    ThreadPool pool(NUM_THREADS);  // ← NEW: create pool once at startup

    replayWAL("wal.log", store);

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return 1;
    }
    if (listen(serverFd, BACKLOG) < 0) {
        perror("listen"); return 1;
    }

    std::cout << "[Server] DistKV listening on port " << PORT
              << " with " << NUM_THREADS << " worker threads.\n";

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t   clientLen = sizeof(clientAddr);
        int clientFd = accept(serverFd, (sockaddr*)&clientAddr, &clientLen);
        if (clientFd < 0) { perror("accept"); continue; }

        std::cout << "[Server] Client: " << inet_ntoa(clientAddr.sin_addr)
                  << "  (queue: " << pool.queueSize() << " pending)\n";

        // ← CHANGED: submit to pool instead of creating a new thread
        pool.submit([clientFd, &store, &wal] {
            clientHandler(clientFd, store, wal);
        });
    }
    close(serverFd);
    return 0;
}