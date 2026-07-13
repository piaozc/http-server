#pragma once

#include <string>

struct ServerConfig {
    std::string environment = "develop";
    int port = 10889;
    int thread_num = 12;
    int sub_reactor_num = 3;
    std::string storage_root = "./storage-dev";
};

class ServerConfigLoader {
public:
    static ServerConfig load(const std::string& path);
};

