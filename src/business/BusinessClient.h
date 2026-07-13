#pragma once

#include "../transfer/TransferTypes.h"

class BusinessClient {
public:
    virtual ~BusinessClient() = default;

    virtual TransferDecision prepareUpload(const TransferRequest& request) = 0;
    virtual TransferDecision prepareDownload(const TransferRequest& request) = 0;
    virtual void reportResult(const TransferResult& result) = 0;
};

