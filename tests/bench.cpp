#include <iostream>
#include <chrono>
#include <string>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int connectToServer(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); return -1;
    }
    return fd;
}

long long sendCmd(int fd, const std::string& cmd) {
    auto start = std::chrono::steady_clock::now();
    send(fd, cmd.c_str(), cmd.size(), 0);
    char buf[256];
    recv(fd, buf, sizeof(buf) - 1, 0);
    auto end = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
}

int main() {
    const int N = 1000;
    const int PORT = 6380;

    int fd = connectToServer(PORT);
    if (fd < 0) return 1;

    std::vector<long long> set_times, get_times;
    std::cout << "Benchmarking " << N << " SET + " << N << " GET...\n";

    for (int i = 0; i < N; i++) {
        std::string cmd = "SET key" + std::to_string(i)
                        + " value" + std::to_string(i) + "\n";
        set_times.push_back(sendCmd(fd, cmd));
    }
    for (int i = 0; i < N; i++) {
        std::string cmd = "GET key" + std::to_string(i) + "\n";
        get_times.push_back(sendCmd(fd, cmd));
    }
    close(fd);

    auto stats = [](std::vector<long long>& v, const std::string& name) {
        std::sort(v.begin(), v.end());
        double avg = std::accumulate(v.begin(), v.end(), 0LL) / (double)v.size();
        std::cout << name << ":\n"
                  << "  avg: " << avg             << " us\n"
                  << "  p50: " << v[v.size()*50/100] << " us\n"
                  << "  p99: " << v[v.size()*99/100] << " us\n"
                  << "  max: " << v.back()         << " us\n";
    };

    stats(set_times, "SET");
    stats(get_times, "GET");
    return 0;
}