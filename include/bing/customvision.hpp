#pragma once

#include <libsoup/soup.h>
#include <QObject>
#include <QImage>

namespace Bing {

class CustomVision : public QObject {
    Q_OBJECT
public:
    CustomVision(QObject *parent = nullptr);
    ~CustomVision();

    void setSubscriptionKey(const QString &subscriptionKey, bool isTrainingKey = false);
    void predict(const QImage &image, const QString &projectId, const QString &iterationId = QString());

private:
    SoupSession * mSession;
    QString       mSubscriptionKey;
    bool          mIsTraining;
};

}