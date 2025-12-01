#include "ASGraph.h"
#include "ASNode.h"

#include <iostream>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <functional>
#include <cctype>
#include "../include/BGP.h"
#include "../include/ROV.h"
#include <stdexcept>


void ASGraph::addNode(const uint32_t asn) {
    if (_node_map.find(asn) == _node_map.end()) {
        auto node = std::make_shared<ASNode>(asn);
        // Ensure each ASNode has a default BGP policy instance
        node->policy = std::make_unique<BGP>();
        _node_map[asn] = node;
    }
}

void ASGraph::setROV(uint32_t asn) {
    addNode(asn);
    _node_map[asn]->policy = std::make_unique<ROV>();
}

void ASGraph::loadROVFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open ROV file " << filename << std::endl;
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        // parse ASN
        try {
            // Trim whitespace and possible CR characters
            size_t start = 0;
            while (start < line.size() && std::isspace((unsigned char)line[start])) ++start;
            size_t end = line.size();
            while (end > start && std::isspace((unsigned char)line[end-1])) --end;
            std::string trimmed = line.substr(start, end - start);

            uint32_t asn = static_cast<uint32_t>(std::stoul(trimmed));
            setROV(asn);
        } catch (...) {
            // ignore malformed lines
        }
    }
}

void ASGraph::loadAnnouncementsFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open announcements file " << filename << std::endl;
        return;
    }

    std::string line;
    // Read header (if present)
    if (!std::getline(file, line)) return;

    // If header doesn't contain expected columns, we'll still attempt to parse rows
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string seed_asn_s, prefix, rov_s;

        // Expect exactly three comma-separated fields: seed_asn,prefix,rov_invalid
        if (!std::getline(ss, seed_asn_s, ',')) continue;
        if (!std::getline(ss, prefix, ',')) continue;
        if (!std::getline(ss, rov_s, ',')) continue;

        // Trim whitespace and CR/LF from parsed fields
        auto trim = [](std::string &s) {
            size_t a = 0;
            while (a < s.size() && std::isspace((unsigned char)s[a])) ++a;
            size_t b = s.size();
            while (b > a && std::isspace((unsigned char)s[b-1])) --b;
            s = s.substr(a, b - a);
        };

        trim(seed_asn_s);
        trim(prefix);
        trim(rov_s);

        try {
            uint32_t seed_asn = static_cast<uint32_t>(std::stoul(seed_asn_s));
            bool rov = (rov_s == "True");
            Announcement ann(prefix, seed_asn);
            ann.rov_invalid = rov;
            seedAnnouncement(seed_asn, ann);
        } catch (...) {
            // ignore malformed lines
            continue;
        }
    }
}

void ASGraph::seedAnnouncement(uint32_t asn, const Announcement& ann) {
    addNode(asn);
    auto node = _node_map.at(asn);
    if (!node->policy) {
        node->policy = std::make_unique<BGP>();
    }

    node->policy->receiveAnnouncement(ann);
    node->policy->processAnnouncements();
}

// Directed cycle detection on provider -> customer edges.
// Returns true if a cycle exists.
bool ASGraph::hasProviderCycle() {
    enum Color { WHITE = 0, GRAY = 1, BLACK = 2 };

    std::unordered_map<uint32_t, int> color;
    color.reserve(_node_map.size());
    for (const auto &p : _node_map) {
        color[p.first] = WHITE;
    }

    std::function<bool(uint32_t)> dfs = [&](uint32_t u) -> bool {
        color[u] = GRAY;
        auto it = _node_map.find(u);
        if (it != _node_map.end()) {
            for (uint32_t v : it->second->_customers) {
                if (color[v] == GRAY) {
                    // back-edge found -> cycle
                    return true;
                } else if (color[v] == WHITE) {
                    if (dfs(v)) return true;
                }
            }
        }
        color[u] = BLACK;
        return false;
    };

    for (const auto &p : _node_map) {
        uint32_t asn = p.first;
        if (color[asn] == WHITE) {
            if (dfs(asn)) return true;
        }
    }
    return false;
}

void ASGraph::addProvider(const uint32_t provider_asn, const uint32_t customer_asn) {
    addNode(provider_asn);
    addNode(customer_asn);

    _node_map[provider_asn]->_customers.push_back(customer_asn);
    _node_map[customer_asn]->_providers.push_back(provider_asn);
}

void ASGraph::addPeer(const uint32_t node1_asn, const uint32_t node2_asn) {
    addNode(node1_asn);
    addNode(node2_asn);
    
    _node_map[node1_asn]->_peers.push_back(node2_asn);
    _node_map[node2_asn]->_peers.push_back(node1_asn);
}

void ASGraph::buildGraphFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::string line;
    // Read the file line by line
    while (std::getline(file, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::stringstream ss(line);
        std::string segment;
        uint32_t node1_asn, node2_asn;
        int relationship;

        // 1. Get node1 ASN
        std::getline(ss, segment, '|');
        // trim
        size_t s1 = 0;
        while (s1 < segment.size() && std::isspace((unsigned char)segment[s1])) ++s1;
        size_t e1 = segment.size();
        while (e1 > s1 && std::isspace((unsigned char)segment[e1-1])) --e1;
        node1_asn = std::stoul(segment.substr(s1, e1 - s1));

        // 2. Get node2 ASN
        std::getline(ss, segment, '|');
        size_t s2 = 0;
        while (s2 < segment.size() && std::isspace((unsigned char)segment[s2])) ++s2;
        size_t e2 = segment.size();
        while (e2 > s2 && std::isspace((unsigned char)segment[e2-1])) --e2;
        node2_asn = std::stoul(segment.substr(s2, e2 - s2));

        // 3. Get relationship type
        std::getline(ss, segment, '|');
        size_t s3 = 0;
        while (s3 < segment.size() && std::isspace((unsigned char)segment[s3])) ++s3;
        size_t e3 = segment.size();
        while (e3 > s3 && std::isspace((unsigned char)segment[e3-1])) --e3;
        relationship = std::stoi(segment.substr(s3, e3 - s3));

        // Process the relationship
        if (relationship == -1) {
            // node1 is provider, node2 is customer
            addProvider(node1_asn, node2_asn);
        } 
        else if (relationship == 0) {
            // node1 and node2 are peers
            addPeer(node1_asn, node2_asn);
        }
    }
}

std::vector<std::vector<uint32_t>> ASGraph::flattenByProviders() {
    if (hasProviderCycle()) {
        throw std::runtime_error("Provider cycle detected in relationships");
    }

    std::unordered_map<uint32_t, int> memo; // asn -> rank
    memo.reserve(_node_map.size());

    std::function<int(uint32_t)> dfs_rank = [&](uint32_t asn) -> int {
        auto itm = memo.find(asn);
        if (itm != memo.end()) return itm->second;

        auto it = _node_map.find(asn);
        if (it == _node_map.end()) {
            // Unknown node, treat as rank 0
            memo[asn] = 0;
            return 0;
        }

        auto node = it->second;
        if (node->_customers.empty()) {
            memo[asn] = 0;
            node->_propagation_rank = 0;
            return 0;
        }

        int mx = 0;
        for (uint32_t c : node->_customers) {
            int r = dfs_rank(c);
            if (r + 1 > mx) mx = r + 1;
        }
        memo[asn] = mx;
        node->_propagation_rank = mx;
        return mx;
    };

    int maxrank = 0;
    for (const auto &p : _node_map) {
        int r = dfs_rank(p.first);
        if (r > maxrank) maxrank = r;
    }

    std::vector<std::vector<uint32_t>> ranks(maxrank + 1);
    for (const auto &p : _node_map) {
        int r = memo[p.first];
        ranks[r].push_back(p.first);
    }

    return ranks;
}

void ASGraph::propagateAnnouncements() {
    // Step 0: prepare ranks
    auto ranks = flattenByProviders();
    if (ranks.empty() && _node_map.empty()) return;
    int maxrank = (int)ranks.size() - 1;

    // UPWARD propagation: from rank 0 up to maxrank
    for (int r = 0; r <= maxrank; ++r) {
        // Send: for each AS in rank r, send its stored local RIB to its providers
        for (uint32_t asn : ranks[r]) {
            auto node_it = _node_map.find(asn);
            if (node_it == _node_map.end()) continue;
            auto node = node_it->second;
            const auto &rib = node->policy->getLocalRIB();
            for (const auto &kv : rib) {
                const std::string &prefix = kv.first;
                const Announcement &stored = kv.second;
                for (uint32_t prov : node->_providers) {
                    // sent announcement: next_hop is the sender (asn), relationship is Customer
                    Announcement sent(prefix, asn, Relationship::Customer, stored.as_path, stored.rov_invalid);
                    _node_map[prov]->policy->receiveAnnouncement(sent);
                }
            }
        }

        // Process: process the next rank (r+1) so providers incorporate received announcements
        if (r + 1 <= maxrank) {
            for (uint32_t asn : ranks[r + 1]) {
                auto node_it = _node_map.find(asn);
                if (node_it == _node_map.end()) continue;
                node_it->second->policy->processAnnouncementsFor(asn);
            }
        }
    }

    // ACROSS (peers): send one hop across peers from all ASes, then process all
    // Send phase
    for (const auto &p : _node_map) {
        uint32_t asn = p.first;
        auto node = p.second;
        const auto &rib = node->policy->getLocalRIB();
        for (const auto &kv : rib) {
            const std::string &prefix = kv.first;
            const Announcement &stored = kv.second;
            for (uint32_t peer : node->_peers) {
                Announcement sent(prefix, asn, Relationship::Peer, stored.as_path, stored.rov_invalid);
                _node_map[peer]->policy->receiveAnnouncement(sent);
            }
        }
    }
    // Process phase: all ASes process their received_queue
    for (const auto &p : _node_map) {
        uint32_t asn = p.first;
        p.second->policy->processAnnouncementsFor(asn);
    }

    // DOWNWARD propagation: from maxrank down to 0
    for (int r = maxrank; r >= 0; --r) {
        // Send from this rank down to customers
        for (uint32_t asn : ranks[r]) {
            auto node_it = _node_map.find(asn);
            if (node_it == _node_map.end()) continue;
            auto node = node_it->second;
            const auto &rib = node->policy->getLocalRIB();
            for (const auto &kv : rib) {
                const std::string &prefix = kv.first;
                const Announcement &stored = kv.second;
                for (uint32_t cust : node->_customers) {
                    Announcement sent(prefix, asn, Relationship::Provider, stored.as_path, stored.rov_invalid);
                    _node_map[cust]->policy->receiveAnnouncement(sent);
                }
            }
        }

        // Process next lower rank (r-1)
        if (r - 1 >= 0) {
            for (uint32_t asn : ranks[r - 1]) {
                auto node_it = _node_map.find(asn);
                if (node_it == _node_map.end()) continue;
                node_it->second->policy->processAnnouncementsFor(asn);
            }
        }
    }
}

void ASGraph::dumpRIBsToCSV(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Error: Could not open output file " << filename << std::endl;
        return;
    }

    out << "asn,prefix,as_path\n";

    std::vector<uint32_t> asns;
    asns.reserve(_node_map.size());
    for (const auto &p : _node_map) asns.push_back(p.first);
    std::sort(asns.begin(), asns.end());

    for (uint32_t asn : asns) {
        auto it = _node_map.find(asn);
        if (it == _node_map.end()) continue;
        auto node = it->second;
        if (!node->policy) continue;
        const auto &rib = node->policy->getLocalRIB();
        for (const auto &kv : rib) {
            const std::string &prefix = kv.first;
            const Announcement &ann = kv.second;

            // Format AS-path as (a, b, c) with a trailing comma for single-element paths: (a,)
            std::ostringstream path_ss;
            path_ss << '(';
            for (size_t i = 0; i < ann.as_path.size(); ++i) {
                if (i) path_ss << ", ";
                path_ss << ann.as_path[i];
            }
            if (ann.as_path.size() == 1) path_ss << ','; // ensure single-element has trailing comma
            path_ss << ')';

            out << asn << ',' << prefix << ',' << '"' << path_ss.str() << '"' << '\n';
        }
    }

    out.close();
}