#include <QGuiApplication>
#include <QQmlApplicationEngine>

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QQmlContext>

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

#include "scoronetwork.h"

#include <QEventLoop>

/*
void testBSON() {
    QString jsonData = "{\"error\":false,\"limit\":50,\"result\":\"4QAAAAMwANkAAAACX2lkAAsAAABuNWlVdUw2QTRLAARidXNlcwC5AAAABDAAGwAAAAEwAIz4Tsx6dUNAATEAtyUcwaCbR0AABDEAGwAAAAEwAJsDBHP0bkNAATEAkKim3dWeR0AABDIAGwAAAAEwALcINvWUb0NAATEA5Spsv6KeR0AABDMAGwAAAAEwABnfwZYxcENAATEA4zmxQOGdR0AABDQAGwAAAAEwAAZKYCz5dUNAATEAgpPD4IeiR0AABDUAGwAAAAEwAAvvchHfeUNAATEAXFoNiXuaR0AAAAAA\",\"skip\":0}";
    QJsonDocument jsonDocument = QJsonDocument::fromJson(jsonData.toLocal8Bit());
    BsonObject bsonObject(QByteArray::fromBase64(jsonDocument.object().value("result").toString().toLatin1()));
    QJsonObject results = QJsonObject::fromVariantMap(bsonObject.toMap());

    // вот и получили координату автобуса :-)
    qDebug() << results["0"];
}

void testRequest() {
    QNetworkRequest request(QUrl("https://api.scorocode.ru/api/v1/data/find"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QString routesRequest = "{\"app\":\"199f6fb0c70a45d6b28d264f81744424\",\"cli\":\"ad553914dd2841bea9dd6a01cb772d7f\",\"coll\":\"buses\",\"fields\":[\"points\"]}";

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QEventLoop *loop = new QEventLoop();
    QObject::connect(manager, SIGNAL(finished(QNetworkReply*)), loop, SLOT(quit()));
    QNetworkReply *reply = manager->post(request, routesRequest.toLatin1());

    while (reply->isRunning()) {
        loop->exec();
    }

    QJsonDocument replyJSON = QJsonDocument::fromJson(reply->readAll());
    qDebug() << replyJSON;
}
*/

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);

    qmlRegisterType<ScoroNetwork>("ru.indigosystem.mobile", 1, 0, "ScoroNetwork");

    QQmlApplicationEngine engine;
    engine.load(QUrl(QLatin1String("qrc:/main.qml")));

    QObject *item = engine.rootObjects().first();
    Q_ASSERT(item);

    QMetaObject::invokeMethod(item, "init");
    QVariantMap parameters;
    /*
    QMetaObject::invokeMethod(item, "init",
        Q_ARG(QVariant, QVariant::fromValue(parameters)));
        */

    return app.exec();    
}
