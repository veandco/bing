#include "bing.hpp"
#include <QCoreApplication>
#include <QImage>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Bing::CustomVision customVision;
    QImage image("0.jpg");

    customVision.setSubscriptionKey("a837a42af0fc4f49acf1bf87ed1dbf12");
    customVision.predict(image, "54fedea0-e8d3-4837-8035-aa05bfbfcbef", "3a8af339-7d3a-4f8e-b7c0-cc118f39d523");

    app.exit();
}