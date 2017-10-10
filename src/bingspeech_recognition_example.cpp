#include "bing.hpp"
#include <sys/stat.h>
#include <cstdio>

int main()
{
    Speech::RecognitionResponse res;
    FILE *file;
    struct stat fileStat;
    char *buf;
    int nread;

    // Initialize Bing
    Bing::init();

    // Initialize Bing Speech
    Speech::init();
    Speech::authenticate("7394827f916d4b48b7a3feb7bfe62aa1");

    // Read raw signed 16-bit 16000hz audio file
    file = fopen("test.raw", "r+");
    if (!file) {
        fprintf(stdout, "Failed to open test.raw\n");
        return 1;
    }
    fstat(fileno(file), &fileStat);
    buf = (char *) malloc(fileStat.st_size);
    nread = fread(buf, 1, fileStat.st_size, file);

    // Recognize text from the audio data
    res = Speech::recognize(buf, nread);
    res.print();

    // Close file
    free(buf);
    fclose(file);
}
