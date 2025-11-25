#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

#include "ASNode.h"
#include "Announcement.h"

class ASGraph {
    std::unordered_map<uint32_t, std::shared_ptr<ASNode>> _node_map; // Maps asn to ASNode object

public:
    auto get(const uint32_t asn) { return _node_map.at(asn); };

    void addNode(const uint32_t asn);
    void addProvider(const uint32_t provider_asn, const uint32_t customer_asn);
    void addPeer(const uint32_t node1_asn, const uint32_t node2_asn);

    void buildGraphFromFile(const std::string& filename);

    bool hasProviderCycle();

    // Flatten graph by provider/customer relation. Returns a vector of vectors of ASNs
    // where index 0 contains ASes with no customers, index 1 contains their providers, etc.
    // Also assigns `_propagation_rank` on each `ASNode`.
    std::vector<std::vector<uint32_t>> flattenByProviders();

    // Seed an announcement directly into the local RIB of the AS `asn`.
    // This will call the AS's Policy `receiveAnnouncement` and then
    // `processAnnouncements` so the announcement becomes the active RIB entry.
    void seedAnnouncement(uint32_t asn, const Announcement& ann);
    // Propagate all announcements through the graph following the
    // three-phase procedure: up, across (peers one hop), then down.
    void propagateAnnouncements();
};
