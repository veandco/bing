#pragma once

#include <libsoup/soup.h>
#include <QByteArray>
#include <QString>
#include <QList>

using namespace std;

namespace Speech {

    void     init();
    void     quit();
    QString  authenticate(const QString &subscriptionKey);
    QString  token();
    QString  fetchToken(const QString &subscriptionKey);
    void     renewToken();
    gboolean onRenewToken(gpointer data);


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

    RecognitionResponse recognize(const QByteArray &data, const QString &lang = "en-US");
    RecognitionResponse parseRecognitionResponse(const QByteArray &data);


    ////////////////
    // Synthesize //
    ////////////////

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

    QByteArray synthesize(const QString &text, Voice::Font font = Voice::en_US::ZiraRUS);
}
