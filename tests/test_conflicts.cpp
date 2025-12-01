// g++ -std=c++17 -I include src/ASGraph.cpp src/BGP.cpp -o tests/run_conflicts tests/test_conflicts.cpp

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

    // Test 1: Path-length tie-breaker
    {
        ASGraph g_conflict;
        g_conflict.addProvider(4u, 3u);
        g_conflict.addProvider(4u, 666u);

        Announcement long_ann("9.9.0.0/16", 9u, Relationship::Customer, std::vector<uint32_t>{9u,8u,3u});
        g_conflict.seedAnnouncement(3u, long_ann);

        Announcement short_ann("9.9.0.0/16", 666u);
        g_conflict.seedAnnouncement(666u, short_ann);

        g_conflict.propagateAnnouncements();

        auto &r4conf = g_conflict.get(4u)->policy->getLocalRIB();
        check(r4conf.find("9.9.0.0/16") != r4conf.end(), "AS4 should have chosen a winner for 9.9.0.0/16");
        if (r4conf.find("9.9.0.0/16") != r4conf.end()) {
            check(r4conf.at("9.9.0.0/16").as_path.size() >= 2 && r4conf.at("9.9.0.0/16").as_path[1] == 666u,
                  "AS4 should prefer the shorter AS-path from AS666 (choose AS666)");
        }
    }

    // Test 2: Relationship precedence (Customer > Peer > Provider)
    {
        ASGraph g_rel;
        g_rel.addProvider(4u, 100u); // 100 is customer of 4
        g_rel.addPeer(4u, 200u);     // 200 peers with 4
        g_rel.addProvider(300u, 4u); // 300 is provider of 4

        Announcement a_cust("8.8.8.0/24", 100u);
        Announcement a_peer("8.8.8.0/24", 200u);
        Announcement a_prov("8.8.8.0/24", 300u);

        g_rel.seedAnnouncement(100u, a_cust);
        g_rel.seedAnnouncement(200u, a_peer);
        g_rel.seedAnnouncement(300u, a_prov);

        g_rel.propagateAnnouncements();

        auto &r4rel = g_rel.get(4u)->policy->getLocalRIB();
        check(r4rel.find("8.8.8.0/24") != r4rel.end(), "AS4 should have chosen a winner for 8.8.8.0/24");
        if (r4rel.find("8.8.8.0/24") != r4rel.end()) {
            check(r4rel.at("8.8.8.0/24").as_path.size() >= 2 && r4rel.at("8.8.8.0/24").as_path[1] == 100u,
                  "AS4 should prefer customer announcements over peer/provider (choose 100)");
        }
    }

    // Test 3: Next hop tie breaker
    {
        ASGraph g_next;
        g_next.addProvider(4u, 10u);
        g_next.addProvider(4u, 20u);

        Announcement a1("7.7.7.0/24", 10u, Relationship::Customer, {10u});
        Announcement a2("7.7.7.0/24", 20u, Relationship::Customer, {20u});

        g_next.seedAnnouncement(10u, a1);
        g_next.seedAnnouncement(20u, a2);

        g_next.propagateAnnouncements();

        auto &r = g_next.get(4u)->policy->getLocalRIB();
        check(r.find("7.7.7.0/24") != r.end(), "AS4 should have chosen a winner for 7.7.7.0/24");
        if (r.find("7.7.7.0/24") != r.end()) {
            check(r.at("7.7.7.0/24").next_hop_asn == 10u,
                "AS4 should choose lower next hop ASN in a tie");
        }
    }

    if (errors == 0) {
        std::cout << "Conflict-resolution tests passed." << std::endl;
        return 0;
    } else {
        std::cerr << errors << " conflict test(s) failed." << std::endl;
        return 1;
    }
}
