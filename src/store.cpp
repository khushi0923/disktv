#include "store.h"
#include <chrono>

KVStore::KVStore(size_t capacity) : capacity_(capacity) {}

bool KVStore::isExpired(const std::string& key) {
    auto it = expiry_.find(key);
    if (it == expiry_.end()) return false;
    return std::chrono::steady_clock::now() > it->second;
}

void KVStore::evictLRU() {
    auto& back = lru_list_.back();
    expiry_.erase(back.key);
    map_.erase(back.key);
    lru_list_.pop_back();
}

void KVStore::moveToFront(std::list<Node>::iterator it) {
    lru_list_.splice(lru_list_.begin(), lru_list_, it);
}

std::string KVStore::ping() {
    return "+PONG\r\n";
}

std::string KVStore::set(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = map_.find(key);
    if (it != map_.end()) {
        it->second->value = value;
        expiry_.erase(key);
        moveToFront(it->second);
    } else {
        if (lru_list_.size() >= capacity_) evictLRU();
        lru_list_.push_front({key, value});
        map_[key] = lru_list_.begin();
    }
    return "+OK\r\n";
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = map_.find(key);
    if (it == map_.end()) return std::nullopt;
    if (isExpired(key)) {
        expiry_.erase(key);
        lru_list_.erase(it->second);
        map_.erase(it);
        return std::nullopt;
    }
    moveToFront(it->second);
    return it->second->value;
}

std::string KVStore::del(const std::string& key) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = map_.find(key);
    if (it == map_.end()) return ":0\r\n";
    expiry_.erase(key);
    lru_list_.erase(it->second);
    map_.erase(it);
    return ":1\r\n";
}

std::string KVStore::expire(const std::string& key, int seconds) {
    std::lock_guard<std::mutex> lock(mtx_);
    auto it = map_.find(key);
    if (it == map_.end()) return ":0\r\n";
    expiry_[key] = std::chrono::steady_clock::now()
                 + std::chrono::seconds(seconds);
    return ":1\r\n";
}