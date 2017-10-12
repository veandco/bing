#include "bing/speech.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Speech {
    const QString FETCH_TOKEN_URI      = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken";
    const QString INTERACTIVE_URL      = "https://speech.platform.bing.com/speech/recognition/interactive/cognitiveservices/v1";
    const QString SYNTHESIZE_URL       = "https://speech.platform.bing.com/synthesize";
    const int     RENEW_TOKEN_INTERVAL = 9; // Minutes before renewing token

    static SoupSession * mSession;
    static QString       mSubscriptionKey;
    static QString       mToken;

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

        mToken.clear();
    }

    QString authenticate(const QString &subscriptionKey)
    {
        mToken.clear();

        mSubscriptionKey = subscriptionKey;
        mToken = Speech::fetchToken(mSubscriptionKey);
        g_timeout_add(Speech::RENEW_TOKEN_INTERVAL * 60 * 1000, Speech::onRenewToken, NULL);
        return mToken;
    }

    QString token()
    {
        return mToken;
    }

    QString fetchToken(const QString &subscriptionKey)
    {
        SoupMessage *msg;
        SoupMessageBody *body;

        msg = soup_message_new("POST", FETCH_TOKEN_URI.toUtf8().data());
        soup_message_headers_append(msg->request_headers, "Content-Length", "0");
        soup_message_headers_append(msg->request_headers, "Ocp-Apim-Subscription-Key", subscriptionKey.toUtf8().data());
        soup_session_send_message(mSession, msg);
        g_object_get(msg, "response-body", &body, NULL);

        return QByteArray(body->data, body->length);
    }

    void renewToken()
    {
        mToken.clear();
        mToken = Speech::fetchToken(mSubscriptionKey);
        fprintf(stdout, "%s\n", "Renewed access token");
    }

    gboolean onRenewToken(void *)
    {
        Speech::renewToken();
        return G_SOURCE_CONTINUE;
    }

    RecognitionResponse recognize(const QByteArray &data, const QString &lang)
    {
        SoupMessage *msg;
        SoupMessageBody *body;
        RecognitionResponse res;
        QString url = INTERACTIVE_URL + "?language=" + lang + "&format=detailed";
        QString auth = "Bearer " + mToken;

        // Do POST request
        msg = soup_message_new("POST", url.toUtf8().data());
        soup_message_set_request(msg, "audio/wav; codec=\"\"audio/pcm\"\"; samplerate=16000", SOUP_MEMORY_COPY, data.data(), data.size());
        soup_message_headers_append(msg->request_headers, "Authorization", auth.toUtf8().data());
        soup_session_send_message(mSession, msg);
        g_object_get(msg, "response-body", &body, NULL);

        return Speech::parseRecognitionResponse(QByteArray(body->data, body->length));
    }

    RecognitionResponse parseRecognitionResponse(const QByteArray &data)
    {
        RecognitionResponse res;
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(data, &error);
        QJsonObject root;

        if (error.error != QJsonParseError::NoError)
            return res;

        root = doc.object();
        res.recognitionStatus = root["RecognitionStatus"].toString();
        res.offset = root["Offset"].toInt();
        res.duration = root["Duration"].toInt();

        auto nbest = root["NBest"].toArray();
        for (auto i = 0; i < nbest.size(); i++) {
            RecognitionResult result;
            QJsonObject item = nbest[i].toObject();

            result.confidence = item["Confidence"].toDouble();
            result.lexical = item["Lexical"].toString();
            result.itn = item["ITN"].toString();
            result.maskedItn = item["MaskedITN"].toString();
            result.display = item["Display"].toString();

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
        fprintf(stdout, "RecognitionStatus: %s\n", recognitionStatus.toUtf8().data());
        fprintf(stdout, "Offset: %d\n", offset);
        fprintf(stdout, "Duration: %d\n", duration);
        fprintf(stdout, "\n");
        for (auto i = 0; i < nbest.size(); i++) {
            fprintf(stdout, "NBest #%d\n", i);
            fprintf(stdout, "-------------------\n");
            fprintf(stdout, "Confidence: %.8f\n", nbest[i].confidence);
            fprintf(stdout, "Lexical: %s\n", nbest[i].lexical.toUtf8().data());
            fprintf(stdout, "ITN: %s\n", nbest[i].itn.toUtf8().data());
            fprintf(stdout, "MaskedITN: %s\n", nbest[i].maskedItn.toUtf8().data());
            fprintf(stdout, "Display: %s\n", nbest[i].display.toUtf8().data());
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

    QByteArray synthesize(const QString &text, Voice::Font font)
    {
        SoupMessage *msg;
        SoupMessageBody *body;
        QString auth = "Bearer " + mToken;
        QString format = "raw-16khz-16bit-mono-pcm";
        QString dataStr = "<speak version='1.0' xml:lang='en-US'><voice xml:lang='" + font.lang + "' xml:gender='" + font.gender + "' name='" + font.name + "'>" + text + "</voice></speak>";
        QByteArray data = dataStr.toUtf8();

        // Do POST request
        msg = soup_message_new("POST", SYNTHESIZE_URL.toUtf8().data());
        soup_message_set_request(msg, "application/ssml+xml", SOUP_MEMORY_COPY, data.data(), data.size());
        soup_message_headers_append(msg->request_headers, "Authorization", auth.toUtf8().data());
        soup_message_headers_append(msg->request_headers, "X-Microsoft-OutputFormat", format.toUtf8().data());
        soup_message_headers_append(msg->request_headers, "User-Agent", "libbing");
        soup_session_send_message(mSession, msg);
        g_object_get(msg, "response-body", &body, NULL);

        return QByteArray(body->data, body->length);
    }
}
