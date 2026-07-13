#pragma once

#include "BusinessClient.h"
#include <string>

class MockBusinessClient : public BusinessClient {
public:
    explicit MockBusinessClient(std::string storage_root);

    TransferDecision prepareUpload(const TransferRequest& request) override;
    TransferDecision prepareDownload(const TransferRequest& request) override;
    void reportResult(const TransferResult& result) override;

private:
    std::string storage_root;

    std::string buildStoragePath(const TransferRequest& request) const;
};

