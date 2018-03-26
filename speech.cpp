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
const QString RECOGNITION_URL      = "https://speech.platform.bing.com/speech/recognition/";
const QString SYNTHESIZE_URL       = "https://speech.platform.bing.com/synthesize";
const int     RENEW_TOKEN_INTERVAL = 9; // Minutes before renewing token

Speech *Speech::mInstance;
SoupSession *Speech::mSession;
QTimer *Speech::mRenewTokenTimer;
QString Speech::mSubscriptionKey;
QString Speech::mToken;
QString Speech::mConnectionId;
QString Speech::mBaseUrl;
bool Speech::mCache;

void Speech::init(int log)
{
    if (mSession)
        return;

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

void Speech::destroy()
{
    if (mSession) {
        g_object_unref(mSession);
        mSession = NULL;
    }

    mToken.clear();
    delete mInstance;
}

Speech *Speech::instance()
{
    if (mInstance)
        return mInstance;

    return new Speech();
}

QString Speech::authenticate(const QString &subscriptionKey)
{
    mSubscriptionKey = subscriptionKey;

    mToken.clear();
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

void Speech::setBaseUrl(const QString &url)
{
    mBaseUrl = url;
}

void Speech::renewToken()
{
    mToken.clear();
    mToken = Speech::fetchToken(mSubscriptionKey);
    fprintf(stdout, "%s\n", "Renewed access token");
}

Speech::RecognitionResponse Speech::recognize(const QByteArray &data, RecognitionLanguage language, RecognitionMode mode)
{
    Speech::RecognitionResponse res;
    SoupMessage *msg;
    SoupMessageBody *body;
    QString modeString;

    switch (mode) {
    default:
    case RecognitionMode::Interactive:
        modeString = "interactive";
        break;
    case RecognitionMode::Dictation:
        modeString = "dictation";
        break;
    case RecognitionMode::Conversation:
        modeString = "conversation";
        break;
    }

    QString url;
    if (mBaseUrl.isEmpty()) {
        url = RECOGNITION_URL + modeString + "/cognitiveservices/v1?language=" + recognitionLanguageString(language) + "&format=detailed";
    } else {
        url = mBaseUrl + modeString + "/cognitiveservices/v1?language=" + recognitionLanguageString(language) + "&format=detailed";
    }
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
    namespace ar_EG {
        Font Hoda {
            "ar-EG",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (ar-EG, Hoda)"
        };
    }

    namespace ar_SA {
        Font Naayf {
            "ar-SA",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (ar-SA, Naayf)"
        };
    }

    namespace bg_BG {
        Font Ivan {
            "bg-BG",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (bg-BG, Ivan)"
        };
    }

    namespace ca_ES {
        Font HerenaRUS {
            "ca-ES",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (ca-ES, HerenaRUS)"
        };
    }

    namespace ca_CZ {
        Font Jakub {
            "ca-CZ",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (cs-CZ, Jakub)"
        };
    }

    namespace da_DK {
        Font HelleRUS {
            "da-DK",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (da-DK, HelleRUS)"
        };
    }

    namespace de_AT {
        Font Michael {
            "de-AT",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (de-AT, Michael)"
        };
    }

    namespace de_CH {
        Font Karsten {
            "de-CH",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (de-CH, Karsten)"
        };
    }

    namespace de_DE {
        Font Hedda {
            "de-DE",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (de-DE, Hedda)"
        };

        Font HeddaRUS {
            "de-DE",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (de-DE, HeddaRUS)"
        };

        Font StefanApollo {
            "de-DE",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (de-DE, Stefan, Apollo)"
        };
    }

    namespace el_GR {
        Font Stefanos {
            "el-GR",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (el-GR, Stefanos)"
        };
    }

    namespace en_AU {
        Font Catherine {
            "en-AU",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-AU, Catherine)"
        };

        Font HayleyRUS {
            "en-AU",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-AU, HayleyRUS)"
        };
    }

    namespace en_CA {
        Font Linda {
            "en-CA",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-CA, Linda)"
        };

        Font HeatherRUS {
            "en-CA",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-CA, HeatherRUS)"
        };
    }

    namespace en_GB {
        Font SusanApollo {
            "en-GB",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-GB, Susan, Apollo)"
        };

        Font HazelRUS {
            "en-GB",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-GB, HazelRUS)"
        };

        Font GeorgeApollo {
            "en-GB",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (en-GB, George, Apollo)"
        };
    }

    namespace en_IE {
        Font Sean {
            "en-IE",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (en-IE, Sean)"
        };
    }

    namespace en_IN {
        Font HeeraApollo {
            "en-IN",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-IN, Heera, Apollo)"
        };

        Font PriyaRUS {
            "en-IN",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-IN, PriyaRUS)"
        };

        Font RaviApollo {
            "en-IN",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (en-IN, Ravi, Apollo)"
        };
    }

    namespace en_US {
        Font ZiraRUS {
            "en-US",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-US, ZiraRUS)"
        };

        Font JessaRUS {
            "en-US",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (en-US, JessaRUS)"
        };

        Font BenjaminRUS {
            "en-US",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (en-US, BenjaminRUS)"
        };
    }

    namespace es_ES {
        Font LauraApollo {
            "es-ES",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (es-ES, Laura, Apollo)"
        };

        Font HelenaRUS {
            "es-ES",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (es-ES, HelenaRUS)"
        };

        Font PabloApollo {
            "es-ES",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (es-ES, Pablo, Apollo)"
        };
    }

    namespace es_MX {
        Font HildaRUS {
            "es-MX",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (es-MX, HildaRUS)"
        };

        Font RaulApollo {
            "es-MX",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (es-MX, Raul, Apollo)"
        };
    }

    namespace fi_FI {
        Font HeidiRUS {
            "fi-FI",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (fi-FI, HeidiRUS)"
        };
    }

    namespace fr_CA {
        Font Caroline {
            "fr-CA",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (fr-CA, Caroline)"
        };

        Font HarmonieRUS {
            "fr-CA",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (fr-CA, HarmonieRUS)"
        };
    }

    namespace fr_CH {
        Font Guillaume {
            "fr-CH",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (fr-CH, Guillaume)"
        };
    }

    namespace fr_FR {
        Font JulieApollo {
            "fr-FR",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (fr-FR, JulieApollo)"
        };

        Font HortenseRUS {
            "fr-FR",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (fr-FR, HortenseRUS)"
        };

        Font PaulApollo {
            "fr-FR",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (fr-FR, PaulApollo)"
        };
    }

    namespace he_IL {
        Font Asaf {
            "he-IL",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (he-IL, Asaf)"
        };
    }

    namespace hi_IN {
        Font KalpanaApollo {
            "hi-IN",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (hi-IN, Kalpana, Apollo)"
        };

        Font Kalpana {
            "hi-IN",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (hi-IN, Kalpana)"
        };

        Font Hemant {
            "hi-IN",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (hi-IN, Hemant)"
        };
    }

    namespace hr_HR {
        Font Matej {
            "hr-HR",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (hr-HR, Matej)"
        };
    }

    namespace hu_HU {
        Font Szabolcs {
            "hu-HU",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (hu-HU, Szabolcs)"
        };
    }

    namespace id_ID {
        Font Andika {
            "id-ID",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (id-ID, Andika)"
        };
    }

    namespace it_IT {
        Font CosimaApollo {
            "it-IT",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (it-IT, Cosimo, Apollo)"
        };
    }

    namespace ja_JP {
        Font AyumiApollo {
            "ja-JP",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (ja-JP, Ayumi, Apollo)"
        };

        Font IchiroApollo {
            "ja-JP",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (ja-JP, Ichiro, Apollo)"
        };

        Font HarukaRUS {
            "ja-JP",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (ja-JP, HarukaRUS)"
        };

        Font LuciaRUS {
            "ja-JP",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (ja-JP, LuciaRUS)"
        };

        Font EkaterinaRUS {
            "ja-JP",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (ja-JP, EkaterinaRUS)"
        };
    }

    namespace ko_KR {
        Font HeamiRUS {
            "ko-KR",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (ko-KR, HeamiRUS)"
        };
    }

    namespace ms_MY {
        Font Rizwan {
            "ms-MY",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (ms-MY, Rizwan)"
        };
    }

    namespace nb_NO {
        Font HuldaRUS {
            "nb-NO",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (nb-NO, HuldaRUS)"
        };
    }

    namespace nl_NL {
        Font HannaRUS {
            "nl-NL",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (nl-NL, HannaRUS)"
        };
    }

    namespace pl_PL {
        Font PaulinaRUS {
            "pl-PL",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (pl-PL, PaulinaRUS)"
        };
    }

    namespace pt_BR {
        Font HeloisaRUS {
            "pt-BR",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (pt-BR, HeloisaRUS)"
        };

        Font DanielApollo {
            "pt-BR",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (pt-BR, DanielApollo)"
        };
    }

    namespace pt_PT {
        Font HeliaRUS {
            "pt-PT",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (pt-PT, HeliaRUS)"
        };
    }

    namespace ro_RO {
        Font Andrei {
            "ro-RO",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (ro-RO, Andrei)"
        };
    }

    namespace ru_RU {
        Font IrinaApollo {
            "ru-RU",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (ru-RU, Irina, Apollo)"
        };

        Font PavelApollo {
            "ru-RU",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (ru-RU, Pavel, Apollo)"
        };
    }

    namespace sk_SK {
        Font Filip {
            "sk-SK",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (sk-SK, Filip)"
        };
    }

    namespace sl_SI {
        Font Lado {
            "sl-SI",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (sl-SI, Lado)"
        };
    }

    namespace sv_SE {
        Font HedvigRUS {
            "sv-SE",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (sv-SE, HedvigRUS)"
        };
    }

    namespace ta_IN {
        Font Valluvar {
            "ta-IN",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (ta-IN, Valluvar)"
        };
    }

    namespace th_TH {
        Font Pattara {
            "th-TH",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (th-TH, Pattara)"
        };
    }

    namespace tr_TR {
        Font SedaRUS {
            "tr-TR",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (tr-TR, SedaRUS)"
        };
    }

    namespace vi_VN {
        Font An {
            "vi-VN",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (vi-VN, An)"
        };
    }

    namespace zh_CN {
        Font HuihuiRUS {
            "zh-CN",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (zh-CN, HuihuiRUS)"
        };

        Font YaoyaoApollo {
            "zh-CN",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (zh-CN, Yaoyao, Apollo)"
        };

        Font KangkangApollo {
            "zh-CN",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (zh-CN, Kangkang, Apollo)"
        };
    }

    namespace zh_HK {
        Font TracyApollo {
            "zh-HK",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (zh-HK, Tracy, Apollo)"
        };

        Font TracyRUS {
            "zh-HK",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (zh-HK, TracyRUS)"
        };

        Font DannyApollo {
            "zh-HK",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (zh-HK, Danny, Apollo)"
        };
    }

    namespace zh_TW {
        Font YatingApollo {
            "zh-TW",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (zh-TW, Yating, Apollo)"
        };

        Font HanHanRUS {
            "zh-TW",
            "Female",
            "Microsoft Server Speech Text to Speech Voice (zh-TW, HanHanRUS)"
        };

        Font ZhiweiApollo {
            "zh-TW",
            "Male",
            "Microsoft Server Speech Text to Speech Voice (zh-TW, Zhiwei, Apollo)"
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

QString Speech::recognitionLanguageString(RecognitionLanguage language)
{
    switch (language) {
    case ArabicEgypt:
        return "ar-EG";
    case CatalanSpain:
        return "ca-ES";
    case DanishDenmark:
        return "da-DK";
    case GermanGermany:
        return "de-DE";
    case EnglishAustralia:
        return "en-AU";
    case EnglishCanada:
        return "en-CA";
    case EnglishUnitedKingdom:
        return "en-GB";
    case EnglishIndia:
        return "en-IN";
    case EnglishNewZealand:
        return "en-NZ";
    default:
    case EnglishUnitedStates:
        return "en-US";
    case SpanishSpain:
        return "es-ES";
    case SpanishMexico:
        return "es-MX";
    case FinnishFinland:
        return "fi-FI";
    case FrenchCanada:
        return "fi-CA";
    case FrenchFrance:
        return "fi-FR";
    case HindiIndia:
        return "hi-IN";
    case ItalianItaly:
        return "it-IT";
    case JapaneseJapan:
        return "ja-JP";
    case KoreanKorea:
        return "ko-KR";
    case NorwegianNorway:
        return "nb-NO";
    case DutchNetherlands:
        return "nl-NL";
    case PolishPoland:
        return "pl-PL";
    case PortugueseBrazil:
        return "pl-BR";
    case PortuguesePortugal:
        return "pl-PT";
    case RussianRussia:
        return "ru-RU";
    case SwedishSweden:
        return "sv-SE";
    case ChineseChina:
        return "zh-CN";
    case ChineseHongKong:
        return "zh-HK";
    case ChineseTaiwan:
        return "zh-TW";
    }
}

}