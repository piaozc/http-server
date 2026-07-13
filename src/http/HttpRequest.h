#pragma once

#include <cstddef>
#include <map>
#include <string>

struct HttpRequest {
    std::string method;
    std::string target;
    std::string path;
    std::string query;
    std::string version;
    std::map<std::string, std::string> headers;
    std::map<std::string, std::string> query_params;
    std::size_t content_length = 0;
};

enum class ParseResult {
    NeedMore,
    Complete,
    BadRequest
};

