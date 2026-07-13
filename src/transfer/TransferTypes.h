#pragma once

#include <cstddef>
#include <string>

struct TransferRequest {
    std::string request_id;
    std::string action;
    std::string user;
    std::string target_user;
    std::string path;
    std::string filename;
    std::size_t size = 0;
};

struct TransferDecision {
    bool allow = false;
    std::string reason;
    std::string storage_path;
    std::string filename;
    std::string content_type = "application/octet-stream";
    std::size_t size = 0;
};

struct TransferResult {
    std::string request_id;
    std::string action;
    bool success = false;
    std::size_t bytes = 0;
    std::string message;
};

