#include "ServerConfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

namespace {

std::string trim(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(begin, end - begin);
}

int parseInt(const std::map<std::string, std::string>& values, const std::string& key, int fallback) {
    auto it = values.find(key);
    if (it == values.end()) {
        return fallback;
    }

    char* end = nullptr;
    long parsed = std::strtol(it->second.c_str(), &end, 10);
    if (end == it->second.c_str() || *end != '\0') {
        std::cerr << "invalid integer config " << key << "=" << it->second << ", use " << fallback << std::endl;
        return fallback;
    }
    return static_cast<int>(parsed);
}

std::string parseString(const std::map<std::string, std::string>& values, const std::string& key, const std::string& fallback) {
    auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}

} // namespace

ServerConfig ServerConfigLoader::load(const std::string& path) {
    ServerConfig config;
    std::ifstream input(path);
    if (!input.is_open()) {
        std::cerr << "config file not found: " << path << ", use default develop config" << std::endl;
        return config;
    }

    std::map<std::string, std::map<std::string, std::string>> sections;
    std::string section = "global";
    std::string line;

    while (std::getline(input, line)) {
        std::size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }

        line = trim(line);
        if (line.empty()) {
            continue;
        }

        if (line.front() == '[' && line.back() == ']') {
            section = trim(line.substr(1, line.size() - 2));
            continue;
        }

        std::size_t equal = line.find('=');
        if (equal == std::string::npos) {
            std::cerr << "ignore invalid config line: " << line << std::endl;
            continue;
        }

        std::string key = trim(line.substr(0, equal));
        std::string value = trim(line.substr(equal + 1));
        sections[section][key] = value;
    }

    config.environment = parseString(sections["global"], "active_environment", config.environment);
    auto env = sections.find(config.environment);
    if (env == sections.end()) {
        std::cerr << "environment config not found: " << config.environment << ", use default develop config" << std::endl;
        return config;
    }

    const auto& values = env->second;
    config.port = parseInt(values, "port", config.port);
    config.thread_num = parseInt(values, "thread_num", config.thread_num);
    config.sub_reactor_num = parseInt(values, "sub_reactor_num", config.sub_reactor_num);
    config.storage_root = parseString(values, "storage_root", config.storage_root);

    return config;
}

