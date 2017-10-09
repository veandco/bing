#pragma once

#include <libsoup/soup.h>
#include <string>
#include <vector>

using namespace std;

namespace Speech {
    struct Result {
        float  confidence;
        string lexical;
        string itn;
        string maskedItn;
        string display;
    };

    struct Response {
        Response();
        ~Response();

        void print() const;

        string         recognitionStatus;
        int            offset;
        int            duration;
        vector<Result> nbest;
    };

    void     init();
    char *   authenticate(const char *subscriptionKey);
    char *   token();
    char *   fetchToken(const char *subscriptionKey);
    void     renewToken();
    gboolean onRenewToken(gpointer data);

    Response recognize(const void *data, int len);
    Response parseResponse(const void *data, int len);
}
