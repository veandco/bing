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
    /////////////////
    // Recognition //
    /////////////////

    struct RecognitionResult {
        double  confidence;
        QString lexical;
        QString itn;
        QString maskedItn;
        QString display;
    };

    struct RecognitionResponse {
        bool hasMatch() const;
        void print() const;

        QString recognitionStatus;
        int     offset;
        int     duration;

        QList<RecognitionResult> nbest;
    };

    ////////////////
    // Synthesize //
    ////////////////

    Speech(int log = 0, QObject *parent = nullptr);
    ~Speech();

    QString authenticate(const QString &subscriptionKey);
    QString token() const;
    QString fetchToken(const QString &subscriptionKey);
    void setCache(bool cache);

    RecognitionResponse recognize(const QByteArray &data, const QString &lang = "en-US");
    QByteArray synthesize(const QString &text, Voice::Font font = Voice::en_US::ZiraRUS);

private:
    SoupSession * mSession;
    QString       mSubscriptionKey;
    QString       mToken;
    QTimer *      mRenewTokenTimer;
    bool          mCache;

    RecognitionResponse parseRecognitionResponse(const QByteArray &data);
    bool hasSynthesizeCache(const QString &text, const Voice::Font &font) const;
    QByteArray loadSynthesizeCache(const QString &text, const Voice::Font &font);
    bool saveSynthesizeCache(const QByteArray &data, const QString &text, const Voice::Font &font);

    static QString cachePath(const QString &text, const Voice::Font &font);

private slots:
    void renewToken();
};

}