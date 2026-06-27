#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <chrono>
#include <mutex>
#include <optional>

struct Node {
    std::string key;
    std::string value;
};

class KVStore {
public:
    explicit KVStore(size_t capacity = 1000);

    std::string set(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key);
    std::string del(const std::string& key);
    std::string expire(const std::string& key, int seconds);
    std::string ping();

private:
    size_t capacity_;
    std::mutex mtx_;

    std::list<Node> lru_list_;
    std::unordered_map<std::string, std::list<Node>::iterator> map_;
    std::unordered_map<std::string,
        std::chrono::steady_clock::time_point> expiry_;

    bool isExpired(const std::string& key);
    void evictLRU();
    void moveToFront(std::list<Node>::iterator it);
};