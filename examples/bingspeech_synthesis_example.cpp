#include "bing.hpp"
#include <sys/stat.h>
#include <cstdio>

int main()
{
    Bing::Speech speech;
    QByteArray output;
    FILE *file;

    // Initialize Bing Speech
    speech.setCache(true);
    speech.authenticate("7394827f916d4b48b7a3feb7bfe62aa1", "7394827f916d4b48b7a3feb7bfe62aa1");

    // Recognize text from the audio data
    output = speech.synthesize("Hi there. Nice to meet you");
    //output = speech.synthesize("大家都指望着我吶", Speech::Voice::zh_CN::HuihuiRUS);

    // Write synthesized audio to a file
    file = fopen("test.raw", "w+");
    if (!file) {
        fprintf(stdout, "Failed to open test.raw\n");
        return 1;
    }
    fwrite(output.data(), 1, output.size(), file);

    // Close file
    fclose(file);
}