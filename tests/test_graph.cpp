// g++ -std=c++17 -I include src/ASGraph.cpp tests/test_graph.cpp -o tests/run_tests

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>

#include "../include/ASGraph.h"
#include "../include/Announcement.h"
#include "../include/Policy.h"

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

    // Test seeding an announcement into an AS local RIB
    Announcement ann("1.2.0.0/16", 1u);
    g.seedAnnouncement(1u, ann);
    auto &rib = g.get(1u)->policy->getLocalRIB();
    check(rib.find("1.2.0.0/16") != rib.end(), "Seeded announcement should appear in AS 1 local RIB");

    // Propagation test: small provider chain 1->2->3 and peer 2<->4
    ASGraph gprop;
    gprop.addProvider(1u, 2u); // 1 is provider of 2
    gprop.addProvider(2u, 3u); // 2 is provider of 3
    gprop.addPeer(2u, 4u);     // 2 peers with 4

    // Seed origin at AS 3
    Announcement pann("5.5.0.0/16", 3u);
    gprop.seedAnnouncement(3u, pann);

    // Confirm AS3 has the origin stored
    auto &r3 = gprop.get(3u)->policy->getLocalRIB();
    check(r3.find("5.5.0.0/16") != r3.end(), "AS3 should have the seeded announcement");
    if (r3.find("5.5.0.0/16") != r3.end()) {
        check(r3.at("5.5.0.0/16").as_path.size() == 1 && r3.at("5.5.0.0/16").as_path[0] == 3u,
              "AS3 stored path should be [3]");
    }

    // Run propagation (up, across, down)
    gprop.propagateAnnouncements();

    // After propagation, AS2 should have path [2,3], AS1 should have [1,2,3]
    auto &r2 = gprop.get(2u)->policy->getLocalRIB();
    auto &r1 = gprop.get(1u)->policy->getLocalRIB();
    auto &r4 = gprop.get(4u)->policy->getLocalRIB();

    check(r2.find("5.5.0.0/16") != r2.end(), "AS2 should have received/stored the announcement");
    if (r2.find("5.5.0.0/16") != r2.end()) {
        check(r2.at("5.5.0.0/16").as_path.size() >= 2 && r2.at("5.5.0.0/16").as_path[0] == 2u,
              "AS2 stored path should start with 2");
    }

    check(r1.find("5.5.0.0/16") != r1.end(), "AS1 should have received/stored the announcement");
    if (r1.find("5.5.0.0/16") != r1.end()) {
        check(r1.at("5.5.0.0/16").as_path.size() >= 3 && r1.at("5.5.0.0/16").as_path[0] == 1u,
              "AS1 stored path should start with 1");
    }

    // AS4 is a peer of AS2 and should have received the announcement across with path starting with 4
    check(r4.find("5.5.0.0/16") != r4.end(), "AS4 (peer of AS2) should have received the announcement via peer step");
    if (r4.find("5.5.0.0/16") != r4.end()) {
        check(r4.at("5.5.0.0/16").as_path.size() >= 2 && r4.at("5.5.0.0/16").as_path[0] == 4u,
              "AS4 stored path should start with 4");
    }

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
