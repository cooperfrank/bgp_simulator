#include <cstdlib>
#include <iostream>

int main() {
    int result = std::system("bunzip2 -f 20250901.as-rel2.txt.bz2"); // Will just run like it was in the terminal

    if (result != 0) {
        std::cerr << "Failed to decompress!" << std::endl;
        return 1;
    }

    std::cout << "Decompressed" << std::endl;
    return 0;
}
