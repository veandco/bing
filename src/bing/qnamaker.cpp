#include "bing/qnamaker.hpp"
#include "bing/exception.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace Bing {

QnaMaker::QnaMaker(int log, QObject *parent) :
    QObject(parent)
{
    SoupLogger *logger;
    SoupLoggerLogLevel logLevel;

    if (log <= 0)
        logLevel = SOUP_LOGGER_LOG_NONE;
    else if (log == 1)
        logLevel = SOUP_LOGGER_LOG_MINIMAL;
    else if (log == 2)
        logLevel = SOUP_LOGGER_LOG_HEADERS;
    else
        logLevel = SOUP_LOGGER_LOG_BODY;

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

void QnaMaker::setKnowledgeBaseId(const QString &knowledgeBaseId)
{
    mKnowledgeBaseId = knowledgeBaseId;
}

QString QnaMaker::generateAnswer(const QString &question, const QString &knowledgeBaseId, int count)
{
    SoupMessage *msg;
    SoupMessageBody *body;
    QString url = "https://westus.api.cognitive.microsoft.com/qnamaker/v2.0/knowledgebases/";

    if (!knowledgeBaseId.isEmpty())
        url += knowledgeBaseId + "/generateAnswer";
    else if (knowledgeBaseId.isEmpty() && !mKnowledgeBaseId.isEmpty())
        url += mKnowledgeBaseId + "/generateAnswer";
    else
        return QString();

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
    int httpStatusCode = soup_session_send_message(mSession, msg);
    if (httpStatusCode >= 400)
        throw Exception(HTTPError);

    // Get answer
    g_object_get(msg, "response-body", &body, NULL);
    QByteArray answerJson(body->data, body->length);
    auto answersObj = QJsonDocument::fromJson(answerJson).object();
    auto answersArray = answersObj["answers"].toArray();
    auto answerObj = answersArray[0].toObject();
    auto answer = answerObj["answer"].toString();
    auto score = answerObj["score"].toDouble();
    if (score > 0)
        return answer;
    else
        return QString();
}

}