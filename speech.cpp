#include "speech.hpp"
#include "exception.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QDebug>
#include <libgen.h>

namespace Bing {

const QString FETCH_TOKEN_URI      = "https://api.cognitive.microsoft.com/sts/v1.0/issueToken";
const QString INTERACTIVE_URL      = "https://speech.platform.bing.com/speech/recognition/interactive/cognitiveservices/v1";
const QString SYNTHESIZE_URL       = "https://speech.platform.bing.com/synthesize";
const int     RENEW_TOKEN_INTERVAL = 9; // Minutes before renewing token

Speech::Speech(int log, QObject *parent) :
    QObject(parent),
    mRenewTokenTimer(nullptr),
    mCache(true)
{
    SoupLogger *logger;
    SoupLoggerLogLevel logLevel;

    if (log <= 0)
        logLevel = SOUP_LOGGER_LOG_NONE;
    else if (log == 1)
        logLevel = SOUP_LOGGER_LOG_MINIMAL;
    else if (log == 2)
        logLevel = SOUP_LOGGER_LOG_HEADERS;
    else
        logLevel = SOUP_LOGGER_LOG_BODY;

    mSession = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER, NULL);
    logger = soup_logger_new(logLevel, -1);
    soup_session_add_feature(mSession, SOUP_SESSION_FEATURE(logger));
    g_object_unref(logger);
}

Speech::~Speech()
{
    if (mSession) {
        g_object_unref(mSession);
        mSession = NULL;
    }

    mToken.clear();
}

QString Speech::authenticate(const QString &subscriptionKey)
{
    mToken.clear();
    mSubscriptionKey = subscriptionKey;
    mToken = Speech::fetchToken(mSubscriptionKey);


    delete mRenewTokenTimer;
    mRenewTokenTimer = new QTimer(this);
    connect(mRenewTokenTimer, &QTimer::timeout, this, &Speech::renewToken);
    mRenewTokenTimer->start(RENEW_TOKEN_INTERVAL * 60 * 1000);

    return mToken;
}

QString Speech::token() const
{
    return mToken;
}

QString Speech::fetchToken(const QString &subscriptionKey)
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

void Speech::setCache(bool cache)
{
    mCache = cache;
}

void Speech::renewToken()
{
    mToken.clear();
    mToken = Speech::fetchToken(mSubscriptionKey);
    fprintf(stdout, "%s\n", "Renewed access token");
}

Speech::RecognitionResponse Speech::recognize(const QByteArray &data, const QString &lang)
{
    Speech::RecognitionResponse res;
    SoupMessage *msg;
    SoupMessageBody *body;
    QString url = INTERACTIVE_URL + "?language=" + lang + "&format=detailed";
    QString auth = "Bearer " + mToken;

    // Do POST request
    msg = soup_message_new("POST", url.toUtf8().data());
    soup_message_set_request(msg, "audio/wav; codec=\"\"audio/pcm\"\"; samplerate=16000", SOUP_MEMORY_COPY, data.data(), data.size());
    soup_message_headers_append(msg->request_headers, "Authorization", auth.toUtf8().data());
    int httpStatusCode = soup_session_send_message(mSession, msg);
    if (httpStatusCode >= 400)
        throw Exception(HTTPError);

    g_object_get(msg, "response-body", &body, NULL);
    return parseRecognitionResponse(QByteArray(body->data, body->length));
}

Speech::RecognitionResponse Speech::parseRecognitionResponse(const QByteArray &data)
{
    Speech::RecognitionResponse res;
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

bool Speech::hasSynthesizeCache(const QString &text, const Voice::Font &font) const
{
    auto path = Speech::cachePath(text, font);
    QFile file(path);

    return file.exists();
}

QByteArray Speech::loadSynthesizeCache(const QString &text, const Voice::Font &font)
{
    QFile file(cachePath(text, font));

    file.open(QIODevice::ReadOnly);
    return file.readAll();
}

bool Speech::saveSynthesizeCache(const QByteArray &data, const QString &text, const Voice::Font &font)
{
    auto path = cachePath(text, font);
    auto pathDup = strdup(path.toUtf8().data());
    QFile file(path);
    QDir dir(QString(dirname(pathDup)));

    free(pathDup);
    if (!dir.exists())
        dir.mkpath(dir.path());

    if (!file.open(QIODevice::WriteOnly))
        return false;

    return file.write(data) >= 0;
}

QString Speech::cachePath(const QString &text, const Voice::Font &font)
{
    QString filePath;

    filePath.append("/var/cache/bing/");
    filePath.append(font.lang + "/");
    filePath.append(font.gender + "/");
    filePath.append(font.name + "/");
    filePath.append(text.trimmed().toLower());

    return filePath;
}

bool Speech::RecognitionResponse::hasMatch() const
{
    return recognitionStatus == "Success";
}

bool Speech::RecognitionResponse::isSilent() const
{
    return recognitionStatus == "InitialSilenceTimeout";
}

void Speech::RecognitionResponse::print() const
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

QByteArray Speech::synthesize(const QString &text, Voice::Font font)
{
    QByteArray result;

    if (mCache && hasSynthesizeCache(text, font))
        return loadSynthesizeCache(text, font);

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

    int httpStatusCode = soup_session_send_message(mSession, msg);
    if (httpStatusCode >= 400)
        throw Exception(HTTPError);

    g_object_get(msg, "response-body", &body, NULL);
    result = QByteArray(body->data, body->length);
    if (mCache) {
        if (!saveSynthesizeCache(result, text, font))
            throw Exception(IOError);
    }

    return result;
}

}