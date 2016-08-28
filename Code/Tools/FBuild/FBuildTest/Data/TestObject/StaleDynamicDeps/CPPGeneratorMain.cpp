
#include <stdio.h>
#include <string.h>

int main(int argc, const char** argv)
{
    if ( argc != 3 )
    {
        printf("Bad Args!\n");
        return 1;
    }

    const char* input = argv[1];
    const char* output = argv[2];

    // Open source - emulate what a generator would do (scanning source file)
    {
        FILE* f = fopen(input, "rb");
        if (!f)
        {
            printf("Failed to open input file '%s'\n", input);
            return 4;
        }
        fclose(f);
    }

    // Generate CPP file which includes header file
    char buffer[1024];
    sprintf(buffer, "#include \"%s\"\n", input);

    // Create output
    {
        FILE* f = fopen(output, "wb");
        if (!f)
        {
            printf("Failed to open output '%s'\n", output);
            return 2;
        }
        if (fwrite(buffer, 1, strlen(buffer), f) != strlen(buffer))
        {
            fclose(f);
            printf("Failed to write to '%s'\n", output);
            return 3;
        }
        fclose(f);
    }

    return 0;
}
