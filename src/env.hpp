#include <fstream>
#include <sstream>
#include <unordered_map>
#include <string>
#include <cstdlib>
#include <iostream>

std::unordered_map<std::string, std::string> loadEnvFile(const std::string& path) {
    std::unordered_map<std::string, std::string> vars;
    std::ifstream file(path);

    if (!file.is_open()) {
        return vars; // empty map if no .env file found
    }

    std::string line;
    while (std::getline(file, line)) {
        // skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        auto eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key   = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        // trim trailing \r in case the file has Windows line endings
        if (!value.empty() && value.back() == '\r') {
            value.pop_back();
        }

        vars[key] = value;
    }

    return vars;
}

std::string getToken(std::string token_id) {
    auto env = loadEnvFile(".env");

    auto tokenIt = env.find(token_id);
    if (tokenIt == env.end()) {
        std::cerr << token_id << " not found in .env file!\n";
        return "-1";
    }

    return tokenIt->second;
}