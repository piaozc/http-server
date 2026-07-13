#pragma once

#include <cstddef>
#include <string>

class HttpResponse {
public:
    static std::string text(int status, const std::string& reason, const std::string& body);
    static std::string downloadHeader(const std::string& filename, const std::string& content_type, std::size_t size);
};

