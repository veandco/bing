#pragma once

#include <libsoup/soup.h>
#include <QObject>

namespace Bing {

class QnaMaker : public QObject {
public:
    QnaMaker(QObject *parent = nullptr);
    ~QnaMaker();

    void setSubscriptionKey(const QString &subscriptionKey);
    QString generateAnswer(const QString &question, const QString &knowledgeBaseId, int count = 1);

private:
    SoupSession * mSession;
    QString       mSubscriptionKey;
};

}