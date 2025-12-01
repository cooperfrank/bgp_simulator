#include <iostream>
#include <string>
#include "../include/ASGraph.h"

// g++ -std=c++17 -I include src/ASGraph.cpp src/BGP.cpp src/main.cpp -o bgp_simulator

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
    std::cout << "Building graph from file..." << std::endl;
    ASGraph g;
    g.buildGraphFromFile(relationships_path);
    std::cout << "Built graph from file." << std::endl;

    // Check for provider cycles and fail early if present
    std::cout << "Checking for cycles in graph..." << std::endl;
    if (g.hasProviderCycle()) {
        std::cerr << "Error: provider/customer relationship cycle detected in " << relationships_path << std::endl;
        return 1;
    }
    std::cout << "Checked for cycles in graph." << std::endl;

    // Load ROV-deploying ASNs
    std::cout << "Loading ROVs from file..." << std::endl;
    g.loadROVFromFile(rov_asns_path);
    std::cout << "Loaded ROVs from file." << std::endl;

    // Load announcements and seed into graph
    std::cout << "Seeding announcements from file..." << std::endl;
    g.loadAnnouncementsFromFile(announcements_path);
    std::cout << "Seeded announcements from file." << std::endl;

    // Run propagation
    std::cout << "Propogating announcements..." << std::endl;
    g.propagateAnnouncements();
    std::cout << "Propogated announcements." << std::endl;

    // Dump resulting RIBs to ribs.csv
    const std::string out = "ribs.csv";
    g.dumpRIBsToCSV(out);
    std::cout << "Wrote " << out << "\n";

    return 0;
}
