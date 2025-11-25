#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <sstream>

#include "../include/ASGraph.h"
#include "../include/Announcement.h"

static void fail(const std::string &msg) {
    std::cerr << "FAILED: " << msg << std::endl;
    std::exit(1);
}

int main() {
    // Test 1: Single AS with one origin announcement
    {
        ASGraph g1;
        Announcement a1("10.0.0.0/24", 1u);
        g1.seedAnnouncement(1u, a1);
        g1.propagateAnnouncements();
        const std::string out1 = "tests/out1.csv";
        g1.dumpRIBsToCSV(out1);

        std::ifstream in(out1);
        if (!in.is_open()) fail("Could not open " + out1);

        std::string line;
        // header
        if (!std::getline(in, line)) fail("Missing header in out1.csv");
        // data line
        if (!std::getline(in, line)) fail("Missing data line in out1.csv");

        // parse asn,prefix,as path
        auto p1 = line.find(',');
        auto p2 = line.find(',', p1 + 1);
        std::string asn_str = line.substr(0, p1);
        std::string prefix = line.substr(p1 + 1, p2 - (p1 + 1));
        std::string path_field = line.substr(p2 + 1);
        // strip quotes
        if (!path_field.empty() && path_field.front() == '"' && path_field.back() == '"') {
            path_field = path_field.substr(1, path_field.size() - 2);
        }

        if (asn_str != "1") fail("Expected ASN 1 in out1.csv, got " + asn_str);
        if (prefix != "10.0.0.0/24") fail("Expected prefix 10.0.0.0/24 in out1.csv, got " + prefix);
        if (path_field != "(1,)") fail("Expected path '(1,)' in out1.csv, got '" + path_field + "'");
    }

    // Test 2: Larger provider chain 1 <- 2 <- 3 (1 is top provider)
    {
        ASGraph g2;
        g2.addProvider(1u, 2u); // 1 provider of 2
        g2.addProvider(2u, 3u); // 2 provider of 3

        Announcement a2("192.0.2.0/24", 3u);
        g2.seedAnnouncement(3u, a2);
        g2.propagateAnnouncements();
        const std::string out2 = "tests/out2.csv";
        g2.dumpRIBsToCSV(out2);

        std::ifstream in(out2);
        if (!in.is_open()) fail("Could not open " + out2);
        std::string line;
        // header
        if (!std::getline(in, line)) fail("Missing header in out2.csv");

        // read all data lines into a vector
        std::vector<std::string> lines;
        while (std::getline(in, line)) {
            if (!line.empty()) lines.push_back(line);
        }

        // We expect entries for ASNs 1,2,3
        if (lines.size() != 3) fail("Expected 3 lines in out2.csv, got " + std::to_string(lines.size()));

        // Because dump sorts ASNs ascending, lines[0] -> ASN 1, lines[1] -> ASN 2, lines[2] -> ASN 3
        auto parse_path = [&](const std::string &l) -> std::string {
            auto p1 = l.find(',');
            auto p2 = l.find(',', p1 + 1);
            std::string path_field = l.substr(p2 + 1);
            if (!path_field.empty() && path_field.front() == '"' && path_field.back() == '"') {
                return path_field.substr(1, path_field.size() - 2);
            }
            return path_field;
        };

        std::string p1_path = parse_path(lines[0]); // ASN 1
        std::string p2_path = parse_path(lines[1]); // ASN 2
        std::string p3_path = parse_path(lines[2]); // ASN 3

        if (p1_path != "(1, 2, 3)") fail("ASN 1 path expected '(1, 2, 3)', got '" + p1_path + "'");
        if (p2_path != "(2, 3)") fail("ASN 2 path expected '(2, 3)', got '" + p2_path + "'");
        if (p3_path != "(3,)") fail("ASN 3 path expected '(3,)', got '" + p3_path + "'");
    }

    // Test 3: Two announcements for the same prefix from different ASes
    {
        ASGraph g3;
        g3.addProvider(4u, 3u);
        g3.addProvider(4u, 666u);

        // Make the announcement seeded at AS3 have a longer AS-path so AS4 prefers ASN 666's shorter path
        Announcement long_ann("9.9.0.0/16", 9u, Relationship::Customer, std::vector<uint32_t>{9u,8u,3u});
        Announcement short_ann("9.9.0.0/16", 666u);

        g3.seedAnnouncement(3u, long_ann);
        g3.seedAnnouncement(666u, short_ann);

        g3.propagateAnnouncements();
        const std::string out3 = "tests/out3.csv";
        g3.dumpRIBsToCSV(out3);

        std::ifstream in(out3);
        if (!in.is_open()) fail("Could not open " + out3);
        std::string line;
        // header
        if (!std::getline(in, line)) fail("Missing header in out3.csv");

        bool found4 = false;
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            auto p1 = line.find(',');
            std::string asn_str = line.substr(0, p1);
            if (asn_str == "4") {
                found4 = true;
                auto p2 = line.find(',', p1 + 1);
                std::string path_field = line.substr(p2 + 1);
                if (!path_field.empty() && path_field.front() == '"' && path_field.back() == '"') {
                    path_field = path_field.substr(1, path_field.size() - 2);
                }
                // path should start with 4 and have 666 as next hop because the shorter route from 666 should be preferred
                // Extract numeric ASNs from the tuple-formatted path '(4, 666)'
                std::vector<std::string> toks;
                std::string cur;
                for (char c : path_field) {
                    if (std::isdigit(static_cast<unsigned char>(c))) {
                        cur.push_back(c);
                    } else if (!cur.empty()) {
                        toks.push_back(cur);
                        cur.clear();
                    }
                }
                if (!cur.empty()) toks.push_back(cur);

                if (toks.size() < 2) fail("ASN 4 path too short in out3.csv");
                if (toks[0] != "4") fail("ASN 4 path does not start with 4, got '" + path_field + "'");
                if (toks[1] != "666") fail("ASN 4 should prefer path via ASN 666, got '" + path_field + "'");
            }
        }
        if (!found4) fail("ASN 4 row missing from out3.csv");
    }

    std::cout << "Output CSV tests passed." << std::endl;
    return 0;
}
