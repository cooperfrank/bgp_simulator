#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

#include "ASNode.h"

class ASGraph {
    std::unordered_map<uint32_t, std::shared_ptr<ASNode>> _node_map; // Maps asn to ASNode object

public:
    auto get(const uint32_t asn) { return _node_map.at(asn); };

    void addNode(const uint32_t asn);
    void addProvider(const uint32_t provider_asn, const uint32_t customer_asn);
    void addPeer(const uint32_t node1_asn, const uint32_t node2_asn);

    void buildGraphFromFile(const std::string& filename);

    bool hasProviderCycle();
};
