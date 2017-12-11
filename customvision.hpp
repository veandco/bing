#pragma once

#include <libsoup/soup.h>
#include <QObject>
#include <QImage>

namespace Bing {

struct Prediction {
    QString tagId;
    QString tag;
    double probability;
};

class CustomVision : public QObject {
    Q_OBJECT
public:
    CustomVision(int log = 0, QObject *parent = nullptr);
    ~CustomVision();

    void setSubscriptionKey(const QString &subscriptionKey, bool isTrainingKey = false);
    QList<Prediction> predict(const QImage &image, const QString &projectId, const QString &iterationId = QString());

private:
    SoupSession * mSession;
    QString       mSubscriptionKey;
    bool          mIsTraining;
};

}