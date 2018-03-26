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

    namespace ar_EG {
        extern Font Hoda;
    }

    namespace ar_SA {
        extern Font Naayf;
    }

    namespace bg_BG {
        extern Font Ivan;
    }

    namespace ca_ES {
        extern Font HerenaRUS;
    }

    namespace cs_CZ {
        extern Font Jakub;
    }

    namespace da_DK {
        extern Font HelleRUS;
    }

    namespace de_AT {
        extern Font Michael;
    }

    namespace de_CH {
        extern Font Karsten;
    }

    namespace de_DE {
        extern Font Hedda;
        extern Font HeddaRUS;
        extern Font StefanApollo;
    }

    namespace el_GR {
        extern Font Stefanos;
    }

    namespace en_AU {
        extern Font Catherine;
        extern Font HayleyRUS;
    }

    namespace en_CA {
        extern Font Linda;
        extern Font HeatherRUS;
    }

    namespace en_GB {
        extern Font SusanApollo;
        extern Font HazelRUS;
        extern Font GeorgeApollo;
    }

    namespace en_IE {
        extern Font Sean;
    }

    namespace en_IN {
        extern Font HeeraApollo;
        extern Font PriyaRUS;
        extern Font RaviApollo;
    }

    namespace en_US {
        extern Font ZiraRUS;
        extern Font JessaRUS;
        extern Font BenjaminRUS;
    }

    namespace en_ES {
        extern Font LauraApollo;
        extern Font HelenaRUS;
        extern Font PabloApollo;
    }

    namespace en_MX {
        extern Font HildaRUS;
        extern Font RaulApollo;
    }

    namespace fi_FI {
        extern Font HeidiRUS;
    }

    namespace fr_CA {
        extern Font Caroline;
        extern Font HarmonieRUS;
    }

    namespace fr_CH {
        extern Font Guillaume;
    }

    namespace fr_FR {
        extern Font JulieApollo;
        extern Font HortenseRUS;
        extern Font PaulApollo;
    }

    namespace he_IL {
        extern Font Asaf;
    }

    namespace hi_IN {
        extern Font KalpanaApollo;
        extern Font Kalpana;
        extern Font Hemant;
    }

    namespace hr_HR {
        extern Font Matej;
    }

    namespace hu_HU {
        extern Font Szabolcs;
    }

    namespace id_ID {
        extern Font Andika;
    }

    namespace it_IT {
        extern Font CosimaApollo;
    }

    namespace ja_JP {
        extern Font AyumiApollo;
        extern Font IchiroApollo;
        extern Font HarukaRUS;
        extern Font LuciaRUS;
        extern Font EkaterinaRUS;
    }

    namespace ko_KR {
        extern Font HeamiRUS;
    }

    namespace ms_MY {
        extern Font Rizwan;
    }

    namespace nb_NO {
        extern Font HuldaRUS;
    }

    namespace nl_NL {
        extern Font HannaRUS;
    }

    namespace pl_PL {
        extern Font PaulinaRUS;
    }

    namespace pt_BR {
        extern Font HeloisaRUS;
        extern Font DanielApollo;
        extern Font HeliaRUS;
    }

    namespace ro_RO {
        extern Font Andrei;
    }

    namespace ru_RU {
        extern Font IrinaApollo;
        extern Font PavelApollo;
    }

    namespace sk_SK {
        extern Font Filip;
    }

    namespace sl_SL {
        extern Font Lado;
    }

    namespace sv_SE {
        extern Font HedvigRUS;
    }

    namespace ta_IN {
        extern Font Valluvar;
    }

    namespace th_TH {
        extern Font Pattara;
    }

    namespace tr_TR {
        extern Font SedaRUS;
    }

    namespace vi_VN {
        extern Font An;
    }

    namespace zh_CN {
        extern Font HuihuiRUS;
        extern Font YaoyaoApollo;
        extern Font KangkangApollo;
    }

    namespace zh_HK {
        extern Font TracyApollo;
        extern Font TracyRUS;
        extern Font DannyApollo;
    }

    namespace zh_TW {
        extern Font YatingApollo;
        extern Font HanHanRUS;
        extern Font ZhiweiApollo;
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
    void setBaseUrl(const QString &url);

    RecognitionResponse recognize(const QByteArray &data, RecognitionLanguage language = EnglishUnitedStates, RecognitionMode mode = Interactive);
    QByteArray synthesize(const QString &text, Voice::Font font = Voice::en_US::ZiraRUS);

private:
    static Speech *mInstance;
    static SoupSession *mSession;
    static QTimer *mRenewTokenTimer;
    static QString mSubscriptionKey;
    static QString mToken;
    static QString mConnectionId;
    static QString mBaseUrl;
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