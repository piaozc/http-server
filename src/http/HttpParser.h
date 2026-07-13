#pragma once

#include "HttpRequest.h"
#include <string>

class HttpParser {
public:
    static ParseResult parseHeaders(const std::string& buffer, HttpRequest* request, std::size_t* header_end);
};

