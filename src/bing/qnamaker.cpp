#include "bing/qnamaker.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Bing {

QnaMaker::QnaMaker(QObject *parent) :
    QObject(parent)
{
    SoupLogger *logger;
    SoupLoggerLogLevel logLevel = SOUP_LOGGER_LOG_BODY;

    mSession = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER, NULL);
    logger = soup_logger_new(logLevel, -1);
    soup_session_add_feature(mSession, SOUP_SESSION_FEATURE(logger));
    g_object_unref(logger);
}

QnaMaker::~QnaMaker()
{
    if (mSession) {
        g_object_unref(mSession);
        mSession = NULL;
    }
}

void QnaMaker::setSubscriptionKey(const QString &subscriptionKey)
{
    mSubscriptionKey = subscriptionKey;
}

QString QnaMaker::generateAnswer(const QString &question, const QString &knowledgeBaseId, int count)
{
    SoupMessage *msg;
    SoupMessageBody *body;
    QString url = "https://westus.api.cognitive.microsoft.com/qnamaker/v2.0/knowledgebases/" + knowledgeBaseId + "/generateAnswer";

    // Build request JSON
    QJsonObject obj;
    obj.insert("question", question);
    obj.insert("top", count);
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson();

    // Send the request
    msg = soup_message_new("POST", url.toUtf8().data());
    soup_message_set_request(msg, "application/json", SOUP_MEMORY_COPY, data.data(), data.size());
    soup_message_headers_append(msg->request_headers, "Ocp-Apim-Subscription-Key", mSubscriptionKey.toUtf8().data());
    soup_session_send_message(mSession, msg);
    g_object_get(msg, "response-body", &body, NULL);

    // Get answer
    QByteArray answerJson(body->data, body->length);
    auto answerObj = QJsonDocument::fromJson(answerJson).object();
    auto answersArray = answerObj["answers"].toArray();
    return answersArray[0].toString();
}

}