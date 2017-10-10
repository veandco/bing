#include "bing.hpp"
#include <sys/stat.h>
#include <cstdio>

int main()
{
    Speech::SynthesizeResponse res;
    FILE *file;

    // Initialize Bing
    Bing::init();

    // Initialize Bing Speech
    Speech::init();
    Speech::authenticate("7394827f916d4b48b7a3feb7bfe62aa1");

    // Recognize text from the audio data
    res = Speech::synthesize("Hi there. Nice to meet you");

    // Write synthesized audio to a file
    file = fopen("test.raw", "w+");
    if (!file) {
        fprintf(stdout, "Failed to open test.raw\n");
        return 1;
    }
    fwrite(res.data, 1, res.length, file);

    // Close file
    fclose(file);

    // Quit Bing Speech
    Speech::quit();

    // Quit Bing
    Bing::quit();
}

