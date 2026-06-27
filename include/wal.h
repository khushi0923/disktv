#pragma once
#include <string>
#include <fstream>

class WAL {
public:
    explicit WAL(const std::string& filepath);
    ~WAL();
    void log(const std::string& command);
    std::string getPath() const;

private:
    std::string filepath_;
    std::ofstream file_;
};