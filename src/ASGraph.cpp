#include "ASGraph.h"
#include "ASNode.h"

#include <iostream>
#include <cstdint>
#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>


void ASGraph::addNode(const uint32_t asn) {
    if (_node_map.find(asn) == _node_map.end()) {
        auto node = std::make_shared<ASNode>(asn);
        _node_map[asn] = node;
    }
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
        node1_asn = std::stoul(segment);

        // 2. Get node2 ASN
        std::getline(ss, segment, '|');
        node2_asn = std::stoul(segment);

        // 3. Get relationship type
        std::getline(ss, segment, '|');
        relationship = std::stoi(segment);

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