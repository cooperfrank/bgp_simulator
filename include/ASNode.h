#pragma once

#include <cstdint>
#include <vector>

class ASNode {
public:
    uint32_t _asn;
    std::vector<uint32_t> _providers;
    std::vector<uint32_t> _customers;
    std::vector<uint32_t> _peers;

    ASNode(uint32_t asn) : _asn(asn) {}
};
