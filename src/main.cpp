#include <iostream>
#include <string>

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


    // Should output a ribs.csv file

    return 0;
}
