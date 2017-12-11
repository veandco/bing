#include "bing.hpp"
#include <sys/stat.h>
#include <cstdio>

int main()
{
    Bing::Speech::init(3);
    auto speech = Bing::Speech::instance();
    Bing::Speech::RecognitionResponse res;
    FILE *file;
    struct stat fileStat;
    char *buf;
    int nread;

    // Initialize Bing Speech
    speech->authenticate("7394827f916d4b48b7a3feb7bfe62aa1");

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
    res = speech->recognize(QByteArray(buf, nread));
    //res = speech.recognize(buf, nread, "zh-CN");
    res.print();

    // Close file
    free(buf);
    fclose(file);

    Bing::Speech::destroy();
}