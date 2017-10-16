#include "bing.hpp"
#include <QCoreApplication>
#include <QtConcurrent>
#include <QDebug>
#include <iostream>

using namespace std;

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Bing::QnaMaker qnaMaker;

    qnaMaker.setSubscriptionKey("e22732045a634d6e84ae3219dc6ec035");
    QtConcurrent::run([&qnaMaker]() {
        string question;

        while (true) {
            cout << "Enter your question: ";
            getline(cin, question);
            if (question.empty())
                continue;

            auto answer = qnaMaker.generateAnswer(QString::fromStdString(question), "8d8e2374-3338-495e-ac74-7657a8b68394");
            if (answer.isEmpty())
                cout << "No answer :(" << endl;
            else
                cout << "Answer:" << answer.toStdString() << endl;
        }
    });

    return app.exec();
}