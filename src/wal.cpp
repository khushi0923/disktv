#include "wal.h"
#include <stdexcept>

WAL::WAL(const std::string& filepath) : filepath_(filepath) {
    file_.open(filepath, std::ios::app);
    if (!file_.is_open())
        throw std::runtime_error("Cannot open WAL file: " + filepath);
}

WAL::~WAL() {
    if (file_.is_open()) file_.close();
}

void WAL::log(const std::string& command) {
    file_ << command << "\n";
    file_.flush();
}

std::string WAL::getPath() const { return filepath_; }