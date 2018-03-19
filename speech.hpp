#pragma once

#include <libsoup/soup.h>
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QList>

class QTimer;

namespace Bing {

namespace Voice {
    struct Font {
        QString lang;
        QString gender;
        QString name;
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

class Speech : public QObject {
    Q_OBJECT
public:
    static void init(int log);
    static void destroy();
    static Speech *instance();

    /////////////////
    // Recognition //
    /////////////////

    enum RecognitionLanguage {
        ArabicEgypt = 0,
        CatalanSpain,
        DanishDenmark,
        GermanGermany,
        EnglishAustralia,
        EnglishCanada,
        EnglishUnitedKingdom,
        EnglishIndia,
        EnglishNewZealand,
        EnglishUnitedStates,
        SpanishSpain,
        SpanishMexico,
        FinnishFinland,
        FrenchCanada,
        FrenchFrance,
        HindiIndia,
        ItalianItaly,
        JapaneseJapan,
        KoreanKorea,
        NorwegianNorway,
        DutchNetherlands,
        PolishPoland,
        PortugueseBrazil,
        PortuguesePortugal,
        RussianRussia,
        SwedishSweden,
        ChineseChina,
        ChineseHongKong,
        ChineseTaiwan,
    };

    enum RecognitionMode {
        Interactive = 0,
        Dictation,
        Conversation,
    };

    struct RecognitionResult {
        double  confidence;
        QString lexical;
        QString itn;
        QString maskedItn;
        QString display;
    };

    struct RecognitionResponse {
        bool hasMatch() const;
        bool isSilent() const;
        void print() const;

        QString recognitionStatus;
        int     offset;
        int     duration;

        QList<RecognitionResult> nbest;
    };

    ////////////////
    // Synthesize //
    ////////////////

    QString authenticate(const QString &subscriptionKey);
    QString token() const;
    QString fetchToken(const QString &subscriptionKey);
    void setCache(bool cache);

    RecognitionResponse recognize(const QByteArray &data, RecognitionLanguage language = EnglishUnitedStates, RecognitionMode mode = Interactive);
    QByteArray synthesize(const QString &text, Voice::Font font = Voice::en_US::ZiraRUS);

private:
    static Speech *mInstance;
    static SoupSession *mSession;
    static QTimer *mRenewTokenTimer;
    static QString mSubscriptionKey;
    static QString mToken;
    static QString mConnectionId;
    static bool mCache;

    static QString cachePath(const QString &text, const Voice::Font &font);
    static QString recognitionLanguageString(RecognitionLanguage language);

    RecognitionResponse parseRecognitionResponse(const QByteArray &data);
    bool hasSynthesizeCache(const QString &text, const Voice::Font &font) const;
    QByteArray loadSynthesizeCache(const QString &text, const Voice::Font &font);
    bool saveSynthesizeCache(const QByteArray &data, const QString &text, const Voice::Font &font);

private slots:
    void renewToken();
};

}