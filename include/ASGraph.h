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

    // Dump the current AS graph local RIBs to CSV with columns:
    // "asn","prefix","as path"
    // "as path" will contain the stored AS-path for the prefix at that AS,
    // ASNs separated by spaces (e.g. "1 2 3").
    void dumpRIBsToCSV(const std::string& filename) const;

    // Mark an ASN as deploying ROV (replace its Policy with an ROV instance)
    void setROV(uint32_t asn);

    // Load ROV-deploying ASNs from a file with one ASN per line
    void loadROVFromFile(const std::string& filename);

    // Load announcements from a CSV with header: seed_asn,prefix,rov_invalid
    // Example rows:
    // 1,10.0.0.0/24,False
    // 2,1.2.0.0/16,True
    void loadAnnouncementsFromFile(const std::string& filename);
};
