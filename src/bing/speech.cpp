#include "bing/speech.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Speech {
    const char * FETCH_TOKEN_URI      = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken";
    const char * INTERACTIVE_URL      = "https://speech.platform.bing.com/speech/recognition/interactive/cognitiveservices/v1";
    const char * SYNTHESIZE_URL       = "https://speech.platform.bing.com/synthesize";
    const int    RENEW_TOKEN_INTERVAL = 9; // Minutes before renewing token

    static SoupSession * mSession;
    static const char *  mSubscriptionKey;
    static char *        mToken;

    void init()
    {
        SoupLogger *logger;
        SoupLoggerLogLevel logLevel = SOUP_LOGGER_LOG_BODY;

        mSession = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER, NULL);
        logger = soup_logger_new(logLevel, -1);
        soup_session_add_feature(mSession, SOUP_SESSION_FEATURE(logger));
        g_object_unref(logger);
    }

    void quit()
    {
        if (mSession) {
            g_object_unref(mSession);
            mSession = NULL;
        }

        if (mToken) {
            free(mToken);
            mToken = NULL;
        }
    }

    char * authenticate(const char *subscriptionKey)
    {
        if (mToken)
            free(mToken);

        mSubscriptionKey = subscriptionKey;
        mToken = Speech::fetchToken(mSubscriptionKey);
        g_timeout_add(Speech::RENEW_TOKEN_INTERVAL * 60 * 1000, Speech::onRenewToken, NULL);
        return mToken;
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
        if (mToken)
            free(mToken);

        mToken = Speech::fetchToken(mSubscriptionKey);
        fprintf(stdout, "%s\n", "Renewed access token");
    }

    gboolean onRenewToken(void *)
    {
        Speech::renewToken();
        return G_SOURCE_CONTINUE;
    }

    RecognitionResponse recognize(const void *data, int len, const char *lang)
    {
        SoupMessage *msg;
        SoupMessageBody *body;
        RecognitionResponse res;
        char url[256] = { 0 };
        char auth[1024] = "Bearer ";

        // Initialize API URL
        sprintf(url, "%s?language=%s&format=detailed", INTERACTIVE_URL, lang);

        // Initialize authorization token
        strcat(auth, mToken);

        // Do POST request
        msg = soup_message_new("POST", url);
        soup_message_set_request(msg, "audio/wav; codec=\"\"audio/pcm\"\"; samplerate=16000", SOUP_MEMORY_COPY, (const char *) data, len);
        soup_message_headers_append(msg->request_headers, "Authorization", auth);
        soup_session_send_message(mSession, msg);
        g_object_get(msg, "response-body", &body, NULL);

        return Speech::parseRecognitionResponse(body->data, body->length);
    }

    RecognitionResponse parseRecognitionResponse(const char *data, int len)
    {
        RecognitionResponse res;
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromRawData(data, len), &error);
        QJsonObject root;

        if (error.error != QJsonParseError::NoError)
            return res;

        root = doc.object();
        res.recognitionStatus = root["RecognitionStatus"].toString().toStdString();
        res.offset = root["Offset"].toInt();
        res.duration = root["Duration"].toInt();

        auto nbest = root["NBest"].toArray();
        for (auto i = 0; i < nbest.size(); i++) {
            RecognitionResult result;
            QJsonObject item = nbest[i].toObject();

            result.confidence = item["Confidence"].toDouble();
            result.lexical = item["Lexical"].toString().toStdString();
            result.itn = item["ITN"].toString().toStdString();
            result.maskedItn = item["MaskedITN"].toString().toStdString();
            result.display = item["Display"].toString().toStdString();

            res.nbest.push_back(result);
        }

        return res;
    }

    bool RecognitionResponse::hasMatch() const
    {
        return recognitionStatus == "Success";
    }

    void RecognitionResponse::print() const
    {
        fprintf(stdout, "RecognitionStatus: %s\n", recognitionStatus.c_str());
        fprintf(stdout, "Offset: %d\n", offset);
        fprintf(stdout, "Duration: %d\n", duration);
        fprintf(stdout, "\n");
        for (size_t i = 0; i < nbest.size(); i++) {
            fprintf(stdout, "NBest #%zd\n", i);
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

    namespace Voice {
        namespace en_US {
            Font ZiraRUS = Font {
                "en-US",
                "Female",
                "Microsoft Server Speech Text to Speech Voice (en-US, ZiraRUS)"
            };

            Font JessaRUS = Font {
                "en-US",
                "Female",
                "Microsoft Server Speech Text to Speech Voice (en-US, JessaRUS)"
            };

            Font BenjaminRUS = Font {
                "en-US",
                "Male",
                "Microsoft Server Speech Text to Speech Voice (en-US, BenjaminRUS)"
            };
        }

        namespace zh_CN {
            Font HuihuiRUS = Font {
                "zh-CN",
                "Female",
                "Microsoft Server Speech Text to Speech Voice (zh-CN, HuihuiRUS)"
            };

            Font YaoyaoApollo = Font {
                "zh-CN",
                "Female",
                "Microsoft Server Speech Text to Speech Voice (zh-CN, Yaoyao, Apollo)"
            };

            Font KangkangApollo = Font {
                "zh-CN",
                "Male",
                "Microsoft Server Speech Text to Speech Voice (zh-CN, Kangkang, Apollo)"
            };
        }
    }

    SynthesizeResponse synthesize(const char *text, Voice::Font font)
    {
        SoupMessage *msg;
        SoupMessageBody *body;
        SynthesizeResponse res;
        char auth[1024] = "Bearer ";
        char format[] = "raw-16khz-16bit-mono-pcm";
        char data[1024] = { 0 };

        // Initialize data
        sprintf(data, "<speak version='1.0' xml:lang='en-US'><voice xml:lang='%s' xml:gender='%s' name='%s'>%s</voice></speak>", font.lang, font.gender, font.name, text);

        // Initialize authorization token
        strcat(auth, mToken);

        // Do POST request
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
