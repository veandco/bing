#include "customvision.hpp"
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Bing {

CustomVision::CustomVision(int log, QObject *parent) :
    QObject(parent)
{
    SoupLogger *logger;
    SoupLoggerLogLevel logLevel = SOUP_LOGGER_LOG_BODY;

    if (log <= 0) {
        logLevel = SOUP_LOGGER_LOG_NONE;
    } else if (log == 1) {
        logLevel = SOUP_LOGGER_LOG_MINIMAL;
    } else if (log == 2) {
        logLevel = SOUP_LOGGER_LOG_HEADERS;
    } else {
        logLevel = SOUP_LOGGER_LOG_BODY;
    }

    mSession = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER, NULL);
    logger = soup_logger_new(logLevel, -1);
    soup_session_add_feature(mSession, SOUP_SESSION_FEATURE(logger));
    g_object_unref(logger);
}

CustomVision::~CustomVision()
{
    if (mSession) {
        g_object_unref(mSession);
        mSession = NULL;
    }
}

void CustomVision::setSubscriptionKey(const QString &subscriptionKey, bool isTrainingKey)
{
    mSubscriptionKey = subscriptionKey;
    mIsTraining = isTrainingKey;
}

QList<Prediction> CustomVision::predict(const QImage &image, const QString &projectId, const QString &iterationId)
{
    SoupMessage *msg;
    SoupMessageBody *body;
    QByteArray imageData;
    QBuffer imageBuffer(&imageData);
    QString url = "https://southcentralus.api.cognitive.microsoft.com/customvision/v1.0/Prediction/" + projectId + "/image";
    QList<Prediction> predictions;

    imageBuffer.open(QIODevice::WriteOnly);
    image.save(&imageBuffer, "JPEG");
    
    if (!iterationId.isEmpty()) {
        url += QString("?iterationId=") + iterationId;
    }

    // Send the request
    msg = soup_message_new("POST", url.toUtf8().data());
    soup_message_set_request(msg, "application/octet-stream", SOUP_MEMORY_COPY, imageData.data(), imageData.size());
    if (!mIsTraining) {
        soup_message_headers_append(msg->request_headers, "Prediction-Key", mSubscriptionKey.toUtf8().data());
    }
    soup_session_send_message(mSession, msg);
    g_object_get(msg, "response-body", &body, NULL);

    QByteArray responseJson(body->data, body->length);
    auto responseObj = QJsonDocument::fromJson(responseJson).object();
    auto predictionsArray = responseObj["Predictions"].toArray();
    for (auto i = 0; i < predictionsArray.size(); i++) {
        auto predictionObj = predictionsArray[i].toObject();
        Prediction prediction;
        prediction.tag = predictionObj["Tag"].toString();
        prediction.tagId = predictionObj["TagId"].toString();
        prediction.probability = predictionObj["Probability"].toDouble();
        predictions.push_back(prediction);
    }

    return predictions;
}

}