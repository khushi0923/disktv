#include "parser.h"
#include <sstream>

std::vector<std::string> parseCommand(const std::string& raw) {
    std::vector<std::string> tokens;
    std::istringstream iss(raw);
    std::string token;
    while (iss >> token) tokens.push_back(token);
    return tokens;
}