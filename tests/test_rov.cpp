#include <iostream>
#include <string>
#include <cstdlib>

#include "../include/ASGraph.h"
#include "../include/Announcement.h"

static void fail(const std::string &msg) {
    std::cerr << "FAILED: " << msg << std::endl;
    std::exit(1);
}

int main() {
    // Test A: ROV AS drops invalid announcements
    {
        ASGraph g;
        // AS2 is provider of AS1
        g.addProvider(2u, 1u);
        // mark AS2 as deploying ROV
        g.setROV(2u);

        // Seed an invalid (hijack) announcement at AS1
        Announcement bad("1.2.0.0/16", 666u, Relationship::Customer, std::vector<uint32_t>{666u}, true);
        g.seedAnnouncement(1u, bad);

        g.propagateAnnouncements();

        // AS2 should have dropped and not stored the announcement
        if (g.get(2u)->policy->getLocalRIB().find("1.2.0.0/16") != g.get(2u)->policy->getLocalRIB().end()) {
            fail("AS2 should not store ROV-invalid announcement");
        }
    }

    // Test B: Non-ROV AS accepts invalid announcement
    {
        ASGraph g;
        g.addProvider(2u, 1u);
        // AS2 is NOT ROV

        Announcement bad("1.2.0.0/16", 666u, Relationship::Customer, std::vector<uint32_t>{666u}, true);
        g.seedAnnouncement(1u, bad);
        g.propagateAnnouncements();

        if (g.get(2u)->policy->getLocalRIB().find("1.2.0.0/16") == g.get(2u)->policy->getLocalRIB().end()) {
            fail("AS2 should store invalid announcement if it is not ROV");
        }
    }

    // Test C: ROV AS accepts valid announcement (rov_invalid=false)
    {
        ASGraph g;
        g.addProvider(2u, 1u);
        g.setROV(2u);

        Announcement good("1.2.0.0/16", 1u);
        g.seedAnnouncement(1u, good);
        g.propagateAnnouncements();

        if (g.get(2u)->policy->getLocalRIB().find("1.2.0.0/16") == g.get(2u)->policy->getLocalRIB().end()) {
            fail("AS2 (ROV) should accept non-invalid announcements");
        }
    }

    std::cout << "ROV tests passed." << std::endl;
    return 0;
}
