#pragma once

#include <libsoup/soup.h>
#include <QObject>

namespace Bing {

class QnaMaker : public QObject {
    Q_OBJECT
public:
    QnaMaker(QObject *parent = nullptr);
    ~QnaMaker();

    void setSubscriptionKey(const QString &subscriptionKey);
    void setKnowledgeBaseId(const QString &knowledgeBaseId);
    QString generateAnswer(const QString &question, const QString &knowledgeBaseId = QString(), int count = 1);

private:
    SoupSession * mSession;
    QString       mSubscriptionKey;
    QString       mKnowledgeBaseId;
};

}