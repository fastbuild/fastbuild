#include <stdio.h>
#include <string.h>

int main(int argc, const char** argv)
{
    if ( argc != 3 )
    {
        printf("Bad Args!\n");
        return 1;
    }

	const char* prefix = argv[1];
    const char* input = argv[2];

	// Scan source files and find any lines that have "//:" in them. In our imaginary build pipeline these are requires header
	// file dependencies that need to be built by FASTBuild. The DependencyProcessor outputs these to stdout and fastbuild will
	// make sure these targets are up to date before finishing the current target.
	{
        FILE* f = fopen(input, "rb");
        if (!f)
        {
            printf("Failed to open input file '%s'\n", input);
            return 2;
        }

		// Read the file line by line and output any lines that start with "//: "
		char buffer[4096];
		while (fgets(buffer, 4096, f) != nullptr)
		{
			if (strlen(buffer) > 3)
			{
				if (buffer[0] == '/' &&
					buffer[1] == '/' &&
					buffer[2] == ':' )
				{
					printf("%s%s", prefix, &buffer[3]);
				}
			}
		}
        fclose(f);
    }
	return 0;
}
