#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>

#include "../include/ASGraph.h"
#include "../include/Announcement.h"

static void fail(const std::string &msg) {
    std::cerr << "FAILED: " << msg << std::endl;
    std::exit(1);
}

int main() {
    const std::string fn = "tests/tmp_anns.csv";

    // Write a small CSV announcements file
    {
        std::ofstream out(fn);
        if (!out.is_open()) fail("Could not write temporary announcements file");
        out << "seed_asn,prefix,rov_invalid\n";
        out << "1,10.0.0.0/24,False\n";
        out << "2,192.0.2.0/24,True\n";
        out << "3,198.51.100.0/24,False\n";
        out.close();
    }

    ASGraph g;
    g.loadAnnouncementsFromFile(fn);

    // Check AS1 stored its prefix (non-ROV invalid)
    if (g.get(1u)->policy->getLocalRIB().find("10.0.0.0/24") == g.get(1u)->policy->getLocalRIB().end()) {
        fail("AS1 should have seeded 10.0.0.0/24");
    }

    // Check AS2 stored its prefix and the ann has rov_invalid=true preserved in the stored announcement
    auto it2 = g.get(2u)->policy->getLocalRIB().find("192.0.2.0/24");
    if (it2 == g.get(2u)->policy->getLocalRIB().end()) {
        fail("AS2 should have seeded 192.0.2.0/24");
    }
    if (!it2->second.rov_invalid) {
        fail("AS2's stored announcement should have rov_invalid=true");
    }

    // Cleanup
    std::remove(fn.c_str());

    std::cout << "Announcement CSV IO test passed." << std::endl;
    return 0;
}
