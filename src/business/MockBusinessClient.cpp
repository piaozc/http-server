#include "MockBusinessClient.h"

#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <utility>

namespace {

std::string cleanSegment(const std::string& value) {
    std::string cleaned;
    for (char c : value) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') {
            cleaned.push_back(c);
        }
    }
    return cleaned.empty() ? "default" : cleaned;
}

std::string cleanPath(const std::string& path) {
    std::stringstream stream(path);
    std::string segment;
    std::string cleaned;
    while (std::getline(stream, segment, '/')) {
        if (segment.empty() || segment == "." || segment == "..") {
            continue;
        }
        cleaned += "/" + cleanSegment(segment);
    }
    return cleaned.empty() ? "/file.bin" : cleaned;
}

void ensureDir(const std::string& path) {
    std::string current;
    for (char c : path) {
        current.push_back(c);
        if (c == '/') {
            mkdir(current.c_str(), 0755);
        }
    }
    mkdir(path.c_str(), 0755);
}

} // namespace

MockBusinessClient::MockBusinessClient(std::string root)
    : storage_root(std::move(root)) {}

TransferDecision MockBusinessClient::prepareUpload(const TransferRequest& request) {
    TransferDecision decision;
    decision.allow = true;
    decision.storage_path = buildStoragePath(request);
    decision.filename = request.filename.empty() ? "file.bin" : request.filename;
    decision.size = request.size;

    std::size_t slash = decision.storage_path.find_last_of('/');
    if (slash != std::string::npos) {
        ensureDir(decision.storage_path.substr(0, slash));
    }
    return decision;
}

TransferDecision MockBusinessClient::prepareDownload(const TransferRequest& request) {
    TransferDecision decision;
    decision.allow = true;
    decision.storage_path = buildStoragePath(request);
    decision.filename = request.filename.empty() ? "download.bin" : request.filename;
    return decision;
}

void MockBusinessClient::reportResult(const TransferResult& result) {
    std::cout << "transfer result request_id=" << result.request_id
              << " action=" << result.action
              << " success=" << result.success
              << " bytes=" << result.bytes
              << " message=" << result.message << std::endl;
}

std::string MockBusinessClient::buildStoragePath(const TransferRequest& request) const {
    std::string owner = cleanSegment(request.target_user.empty() ? request.user : request.target_user);
    return storage_root + "/" + owner + cleanPath(request.path);
}
