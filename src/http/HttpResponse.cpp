#include "HttpResponse.h"

#include <sstream>

std::string HttpResponse::text(int status, const std::string& reason, const std::string& body) {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << " " << reason << "\r\n"
             << "Content-Type: text/plain; charset=utf-8\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
    return response.str();
}

std::string HttpResponse::downloadHeader(const std::string& filename, const std::string& content_type, std::size_t size) {
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << size << "\r\n"
             << "Content-Disposition: attachment; filename=\"" << filename << "\"\r\n"
             << "Connection: close\r\n"
             << "\r\n";
    return response.str();
}

