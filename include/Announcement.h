#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class Relationship {
    Origin,    // announcement started here
    Provider,  // received from a provider
    Peer,      // received from a peer
    Customer   // received from a customer
};

struct Announcement {
    std::string prefix;             // e.g., "8.8.8.0/24"
    std::vector<uint32_t> as_path;  // AS path as a list of ASNs
    uint32_t next_hop_asn;          // where announcement came from
    Relationship received_from;     // origin, provider, peer, or customer

    // Constructor for origin announcements
    Announcement(const std::string& p, uint32_t origin_asn)
        : prefix(p), as_path(), next_hop_asn(origin_asn), received_from(Relationship::Origin) {
        as_path.push_back(origin_asn); // start AS path with origin
    }

    // Constructor for received announcements
    Announcement(const std::string& p, uint32_t nh, Relationship rel, const std::vector<uint32_t>& path)
        : prefix(p), next_hop_asn(nh), received_from(rel), as_path(path) {}
};
