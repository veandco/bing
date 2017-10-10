#pragma once

#include <libsoup/soup.h>
#include <string>
#include <vector>

using namespace std;

namespace Speech {

    void     init();
    char *   authenticate(const char *subscriptionKey);
    char *   token();
    char *   fetchToken(const char *subscriptionKey);
    void     renewToken();
    gboolean onRenewToken(gpointer data);


    /////////////////
    // Recognition //
    /////////////////

    struct RecognitionResult {
        float  confidence;
        string lexical;
        string itn;
        string maskedItn;
        string display;
    };

    struct RecognitionResponse {
        RecognitionResponse();
        ~RecognitionResponse();

        void                      print() const;

        string                    recognitionStatus;
        int                       offset;
        int                       duration;
        vector<RecognitionResult> nbest;
    };

    RecognitionResponse recognize(const void *data, int len);
    RecognitionResponse parseRecognitionResponse(const void *data);


    ////////////////
    // Synthesize //
    ////////////////

    struct SynthesizeResponse {
        const char * data;
        int          length;
    };

    SynthesizeResponse synthesize(const char *text);
}
