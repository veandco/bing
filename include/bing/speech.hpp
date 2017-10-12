#pragma once

#include <libsoup/soup.h>
#include <string>
#include <vector>

using namespace std;

namespace Speech {

    void     init();
    void     quit();
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
        bool hasMatch() const;
        void print() const;

        string recognitionStatus;
        int    offset;
        int    duration;

        vector<RecognitionResult> nbest;
    };

    RecognitionResponse recognize(const void *data, int len, const char *lang = "en-US");
    RecognitionResponse parseRecognitionResponse(const void *data);


    ////////////////
    // Synthesize //
    ////////////////

    namespace Voice {
        struct Font {
            const char *lang;
            const char *gender;
            const char *name;
        };

        namespace en_US {
            extern Font ZiraRUS;
            extern Font JessaRUS;
            extern Font BenjaminRUS;
        }

        namespace zh_CN {
            extern Font HuihuiRUS;
            extern Font YaoyaoApollo;
            extern Font KangkangApollo;
        }
    }

    struct SynthesizeResponse {
        const char * data;
        int          length;
    };

    SynthesizeResponse synthesize(const char *text, Voice::Font font = Voice::en_US::ZiraRUS);
}
