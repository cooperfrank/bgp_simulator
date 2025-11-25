#pragma once

#include <cstdint>
#include <vector>
#include <memory>

// Forward-declare Policy to avoid including Policy.h in this header
class Policy;


class ASNode {
public:
    uint32_t _asn;
    std::vector<uint32_t> _providers;
    std::vector<uint32_t> _customers;
    std::vector<uint32_t> _peers;
    std::unique_ptr<Policy> policy;
    // Propagation rank used when flattening the provider/customer DAG.
    // Nodes with no customers have rank 0. Higher ranks are further "up" the provider chain.
    int _propagation_rank = -1;

    ASNode(uint32_t asn) : _asn(asn) {}
};
