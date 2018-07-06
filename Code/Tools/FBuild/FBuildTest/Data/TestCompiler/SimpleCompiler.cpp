//
// An simple compiler that takes an input filename and output filename and creates the output file
//
#include <stdio.h>

int main(int argc, char ** argv)
{
    if (argc != 3)
    {
        printf("Bad Args!\n");
        return 1;
    }

    //const char* input = argv[1];
    const char* output = argv[2];

    FILE* f = fopen(output, "wb");
    fwrite((char*)&argc, sizeof(argc), 1, f);
    fclose(f);
    return 0;
}
