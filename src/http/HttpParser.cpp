#include "HttpParser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
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

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

void parseTarget(HttpRequest* request) {
    std::size_t question = request->target.find('?');
    if (question == std::string::npos) {
        request->path = request->target;
        return;
    }

    request->path = request->target.substr(0, question);
    request->query = request->target.substr(question + 1);

    std::stringstream query_stream(request->query);
    std::string pair;
    while (std::getline(query_stream, pair, '&')) {
        std::size_t equal = pair.find('=');
        if (equal == std::string::npos) {
            request->query_params[pair] = "";
        } else {
            request->query_params[pair.substr(0, equal)] = pair.substr(equal + 1);
        }
    }
}

} // namespace

ParseResult HttpParser::parseHeaders(const std::string& buffer, HttpRequest* request, std::size_t* header_end) {
    std::size_t end = buffer.find("\r\n\r\n");
    if (end == std::string::npos) {
        return ParseResult::NeedMore;
    }

    *header_end = end + 4;
    std::istringstream stream(buffer.substr(0, end));

    std::string request_line;
    if (!std::getline(stream, request_line)) {
        return ParseResult::BadRequest;
    }
    if (!request_line.empty() && request_line.back() == '\r') {
        request_line.pop_back();
    }

    std::istringstream request_line_stream(request_line);
    if (!(request_line_stream >> request->method >> request->target >> request->version)) {
        return ParseResult::BadRequest;
    }
    parseTarget(request);

    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::size_t colon = line.find(':');
        if (colon == std::string::npos) {
            return ParseResult::BadRequest;
        }

        request->headers[lower(line.substr(0, colon))] = trim(line.substr(colon + 1));
    }

    auto content_length = request->headers.find("content-length");
    if (content_length != request->headers.end()) {
        char* parse_end = nullptr;
        unsigned long long parsed = std::strtoull(content_length->second.c_str(), &parse_end, 10);
        if (parse_end == content_length->second.c_str() || *parse_end != '\0') {
            return ParseResult::BadRequest;
        }
        request->content_length = static_cast<std::size_t>(parsed);
    }

    return ParseResult::Complete;
}

