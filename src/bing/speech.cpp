#include "bing/speech.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <json/json.h>
#include <sstream>

namespace Speech {
    const char *         FETCH_TOKEN_URI      = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken";
    const char *         INTERACTIVE_URL      = "https://speech.platform.bing.com/speech/recognition/interactive/cognitiveservices/v1";
    const char *         SYNTHESIZE_URL       = "https://speech.platform.bing.com/synthesize";
    const int            RENEW_TOKEN_INTERVAL = 9; // Minutes before renewing token

    static SoupSession * mSession;
    static const char *  mSubscriptionKey;
    static char *        mToken;
    static guint         mCallbackId;

    void init()
    {
        SoupLogger *logger;
        SoupLoggerLogLevel logLevel = SOUP_LOGGER_LOG_BODY;

        mSession = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER, NULL);
        logger = soup_logger_new(logLevel, -1);
        soup_session_add_feature(mSession, SOUP_SESSION_FEATURE(logger));
        g_object_unref(logger);
    }

    char * authenticate(const char *subscriptionKey)
    {
        if (mToken)
            free(mToken);

        mSubscriptionKey = subscriptionKey;
        mToken = Speech::fetchToken(mSubscriptionKey);
        g_timeout_add(Speech::RENEW_TOKEN_INTERVAL * 60 * 1000, Speech::onRenewToken, NULL);
    }

    char * token()
    {
        return mToken;
    }

    char * fetchToken(const char *subscriptionKey)
    {
        SoupMessage *msg;
        SoupMessageBody *body;
        char *token = NULL;

        msg = soup_message_new("POST", FETCH_TOKEN_URI);
        soup_message_headers_append(msg->request_headers, "Content-Length", "0");
        soup_message_headers_append(msg->request_headers, "Ocp-Apim-Subscription-Key", subscriptionKey);
        soup_session_send_message(mSession, msg);
        g_object_get(msg, "response-body", &body, NULL);
        token = strdup(body->data);

        return token;
    }

    void renewToken()
    {
        mToken = Speech::fetchToken(mSubscriptionKey);
        fprintf(stdout, "%s\n", "Renewed access token");
    }

    gboolean onRenewToken(gpointer data)
    {
        Speech::renewToken();
    }

    Response recognize(const void *data, int len)
    {
        SoupMessage *msg;
        SoupMessageBody *body;
        Response res;
        char url[256] = { 0 };
        char auth[1024] = "Bearer ";

        // Initialize API URL
        strcpy(url, INTERACTIVE_URL);
        strcat(url, "?language=en-US&format=detailed");

        // Initialize authorization token
        strcat(auth, mToken);

        msg = soup_message_new("POST", url);
        soup_message_set_request(msg, "audio/wav; codec=\"\"audio/pcm\"\"; samplerate=16000", SOUP_MEMORY_COPY, (const char *) data, len);
        soup_message_headers_append(msg->request_headers, "Authorization", auth);
        soup_session_send_message(mSession, msg);
        g_object_get(msg, "response-body", &body, NULL);

        return Speech::parseResponse(body->data, body->length);
    }

    Response parseResponse(const void *data, int len)
    {
        using namespace std;
        using namespace Json;

        Response res;
        CharReaderBuilder builder;
        string s((char *) data);
        string err;
        istringstream in(s);
        Value root;

        if (!parseFromStream(builder, in, &root, &err)) {
            fprintf(stderr, "parseResponse(): %s\n", err.c_str());
            return res;
        }

        res.recognitionStatus = root["RecognitionStatus"].asString();
        res.offset = root["Offset"].asInt();
        res.duration = root["Duration"].asInt();
        root = root["NBest"];
        if (root.isArray()) {
            for (int i = 0; i < root.size(); i++) {
                Value item = root[i];
                Result result;
                
                result.confidence = item["Confidence"].asFloat();
                result.lexical = item["Lexical"].asString();
                result.itn = item["ITN"].asString();
                result.maskedItn = item["MaskedITN"].asString();
                result.display = item["Display"].asString();
                res.nbest.push_back(result);
            }
        }
    }

    Response::Response()
    {
    }

    Response::~Response()
    {
    }

    void Response::print() const
    {
        fprintf(stdout, "RecognitionStatus: %s\n", recognitionStatus.c_str());
        fprintf(stdout, "Offset: %d\n", offset);
        fprintf(stdout, "Duration: %d\n", duration);
        fprintf(stdout, "\n");
        for (int i = 0; i < nbest.size(); i++) {
            fprintf(stdout, "NBest #%d\n", i);
            fprintf(stdout, "-------------------\n");
            fprintf(stdout, "Confidence: %.8f\n", nbest[i].confidence);
            fprintf(stdout, "Lexical: %s\n", nbest[i].lexical.c_str());
            fprintf(stdout, "ITN: %s\n", nbest[i].itn.c_str());
            fprintf(stdout, "MaskedITN: %s\n", nbest[i].maskedItn.c_str());
            fprintf(stdout, "Display: %s\n", nbest[i].display.c_str());
            fprintf(stdout, "\n");
        }
        fprintf(stdout, "\n");
    }


    ////////////////
    // Synthesize //
    ////////////////

    SynthesizeResponse synthesize(const char *text)
    {
        SoupMessage *msg;
        SoupMessageBody *body;
        SynthesizeResponse res;
        char auth[1024] = "Bearer ";
        char format[] = "raw-16khz-16bit-mono-pcm";
        char data[1024] = "<speak version='1.0' xml:lang='en-US'><voice xml:lang='en-US' xml:gender='Female' name='Microsoft Server Speech Text to Speech Voice (en-US, ZiraRUS)'>";

        // Initialize data
        strcat(data, text);
        strcat(data, "</voice></speak>");

        // Initialize authorization token
        strcat(auth, mToken);

        msg = soup_message_new("POST", SYNTHESIZE_URL);
        soup_message_set_request(msg, "application/ssml+xml", SOUP_MEMORY_COPY, data, strlen(data));
        soup_message_headers_append(msg->request_headers, "Authorization", auth);
        soup_message_headers_append(msg->request_headers, "X-Microsoft-OutputFormat", format);
        soup_message_headers_append(msg->request_headers, "User-Agent", "libbing");
        soup_session_send_message(mSession, msg);
        g_object_get(msg, "response-body", &body, NULL);
        res.data = body->data;
        res.length = body->length;

        return res;
    }
}
