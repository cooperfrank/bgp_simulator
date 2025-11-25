#include <iostream>
#include <string>
#include "../include/ASGraph.h"

// ./bgp_simulator 
// --relationships ../bench/many/CAIDAASGraphCollector_2025.10.16.txt 
// --announcements ../bench/many/anns.csv 
// --rov-asns ../bench/many/rov_asns.csv

int main(int argc, char* argv[]) {
    if (argc != 7) {
        std::cerr << "Usage: " 
                  << argv[0]
                  << " --relationships <path> --announcements <path> --rov-asns <path>\n";
        return 1;
    }

    std::string relationships_path;
    std::string announcements_path;
    std::string rov_asns_path;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--relationships" && i + 1 < argc) {
            relationships_path = argv[++i];
        } else if (arg == "--announcements" && i + 1 < argc) {
            announcements_path = argv[++i];
        } else if (arg == "--rov-asns" && i + 1 < argc) {
            rov_asns_path = argv[++i];
        } else {
            std::cerr << "Unknown or incomplete argument: " << arg << "\n";
            return 1;
        }
    }

    std::cout << "Relationships: " << relationships_path << "\n";
    std::cout << "Announcements: " << announcements_path << "\n";
    std::cout << "ROV ASNs: " << rov_asns_path << "\n";

    // Build graph from relationships
    ASGraph g;
    g.buildGraphFromFile(relationships_path);

    // Load ROV-deploying ASNs
    g.loadROVFromFile(rov_asns_path);

    // Load announcements and seed into graph
    g.loadAnnouncementsFromFile(announcements_path);

    // Run propagation
    g.propagateAnnouncements();

    // Dump resulting RIBs to ribs.csv
    const std::string out = "ribs.csv";
    g.dumpRIBsToCSV(out);
    std::cout << "Wrote " << out << "\n";

    return 0;
}
