#include "bing.hpp"
#include <QDebug>

int main()
{
    Bing::QnaMaker qnaMaker;

    qnaMaker.setSubscriptionKey("e22732045a634d6e84ae3219dc6ec035");
    auto answer = qnaMaker.generateAnswer("hi", "8d8e2374-3338-495e-ac74-7657a8b68394");

    qDebug() << answer;
}