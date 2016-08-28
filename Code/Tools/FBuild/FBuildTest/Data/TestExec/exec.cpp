//
// An simple executable to run
//
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char *argv[], char *[])
{
    // Touch each file listed
    for (int i = 1; i < argc; ++i)
    {
        const char * arg = argv[i];

        // Make a new filename based on the input
        std::string outFileName = arg;
        outFileName += ".out";

        // Touch the file
        std::ofstream file;
        file.open(outFileName);
        file << "T";
        file.close();

        // Generate some output
        // on STDOUT based on the arguments too
        std::cout << "Touched: " << outFileName << std::endl;
    }

    return argc - 1;
}

