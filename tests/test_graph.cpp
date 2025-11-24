// g++ -std=c++17 -I include src/ASGraph.cpp tests/test_graph.cpp -o tests/run_tests

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>

#include "../include/ASGraph.h"

int main() {
    int errors = 0;

    auto check = [&](bool cond, const std::string& msg) {
        if (!cond) {
            std::cerr << "FAILED: " << msg << std::endl;
            ++errors;
        }
    };

    // Test node creation
    ASGraph g;
    g.addNode(100u);
    auto n100 = g.get(100u);
    check(n100 != nullptr, "Node 100 should exist after addNode");
    if (n100) check(n100->_asn == 100u, "Node 100 ASN should be 100");

    // Test provider/customer relationship
    g.addProvider(1u, 2u); // 1 is provider of 2
    auto n1 = g.get(1u);
    auto n2 = g.get(2u);
    check(n1 != nullptr && n2 != nullptr, "Nodes 1 and 2 should exist after addProvider");
    if (n1) check(!n1->_customers.empty() && n1->_customers[0] == 2u, "Node 1 should have customer 2");
    if (n2) check(!n2->_providers.empty() && n2->_providers[0] == 1u, "Node 2 should have provider 1");

    // Test peer relationship
    g.addPeer(3u, 4u);
    auto n3 = g.get(3u);
    auto n4 = g.get(4u);
    check(n3 != nullptr && n4 != nullptr, "Nodes 3 and 4 should exist after addPeer");
    if (n3) check(!n3->_peers.empty() && n3->_peers[0] == 4u, "Node 3 should peer with 4");
    if (n4) check(!n4->_peers.empty() && n4->_peers[0] == 3u, "Node 4 should peer with 3");

    // Test buildGraphFromFile
    ASGraph g2;
    const std::string fname = "tests/sample_edges.txt";
    g2.buildGraphFromFile(fname);

    auto n5 = g2.get(5u);
    auto n6 = g2.get(6u);
    auto n7 = g2.get(7u);
    auto n8 = g2.get(8u);

    check(n5 != nullptr && n6 != nullptr, "Nodes 5 and 6 should be created from file");
    if (n5) check(!n5->_customers.empty() && n5->_customers[0] == 6u, "Node 5 should have customer 6 from file");
    if (n6) check(!n6->_providers.empty() && n6->_providers[0] == 5u, "Node 6 should have provider 5 from file");

    check(n7 != nullptr && n8 != nullptr, "Nodes 7 and 8 should be created from file");
    if (n7) check(!n7->_peers.empty() && n7->_peers[0] == 8u, "Node 7 should peer with 8 from file");
    if (n8) check(!n8->_peers.empty() && n8->_peers[0] == 7u, "Node 8 should peer with 7 from file");

    // Ensure sample file graph has no provider cycles
    check(!g2.hasProviderCycle(), "Sample-edge graph should not contain provider cycles");

    // Create an explicit no-cycle chain and check false
    ASGraph g_no_cycle;
    g_no_cycle.addProvider(20u, 21u);
    g_no_cycle.addProvider(21u, 22u);
    check(!g_no_cycle.hasProviderCycle(), "Simple provider chain should not be a cycle");

    // Create a cycle 10 -> 11 -> 12 -> 10
    ASGraph g_cycle;
    g_cycle.addProvider(10u, 11u);
    g_cycle.addProvider(11u, 12u);
    g_cycle.addProvider(12u, 10u);
    check(g_cycle.hasProviderCycle(), "Provider cycle should be detected (10->11->12->10)");

    if (errors == 0) {
        std::cout << "All tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << errors << " test(s) failed." << std::endl;
        return 1;
    }
}
