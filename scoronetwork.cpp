#include "scoronetwork.h"

#include <QDebug>
#include <QNetworkReply>
#include <QStandardItem>

#include "busmodel.h"
#include "bson/qt-bson_p.h"

ScoroNetwork::ScoroNetwork(QObject *parent) : QObject(parent)
{
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), SLOT(replyFinished(QNetworkReply*)));

    updateTimer = new QTimer(this);
    updateTimer->setInterval(20 * 1000);
    connect(updateTimer, SIGNAL(timeout()), SLOT(updateTimerTick()));

    apiUrl = "https://api.scorocode.ru/api/v1/data/find";

    m_busModel = new BusModel(this);
}

ScoroNetwork::~ScoroNetwork() {
}

void ScoroNetwork::getRoutesAndPoints() {
    lastQueryType = GET_ROUTES;

    QNetworkRequest request(QUrl("https://api.scorocode.ru/api/v1/data/find"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString routesRequest = QString("{\"app\":\"199f6fb0c70a45d6b28d264f81744424\",\"cli\":\"ad553914dd2841bea9dd6a01cb772d7f\",\"coll\":\"buses\",\"query\":{\"name\":{\"$eq\":\"%1\"}},\"fields\":[\"points\"]}").arg(m_routeName);
    currentReply = manager->post(request, routesRequest.toLatin1());
}

void ScoroNetwork::updateTimerTick() {
    lastQueryType = GET_BUSES;

    QNetworkRequest request(QUrl("https://api.scorocode.ru/api/v1/data/find"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    QString routesRequest = QString("{\"app\":\"199f6fb0c70a45d6b28d264f81744424\",\"cli\":\"ad553914dd2841bea9dd6a01cb772d7f\",\"coll\":\"buses\",\"query\":{\"name\":{\"$eq\":\"%1\"}},\"fields\":[\"buses\"]}").arg(m_routeName);
    currentReply = manager->post(request, routesRequest.toLatin1());
}

void ScoroNetwork::replyFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError)
        return;

    QString replyString = QString::fromLatin1(reply->readAll());

    if (replyString == "")
        return;

    QJsonDocument replyJSON = QJsonDocument::fromJson(replyString.toLatin1());

    QByteArray bsonData = QByteArray::fromBase64(replyJSON.object().value("result").toString().toLatin1());
    BsonObject bsonObject(bsonData);
    QJsonObject result = QJsonObject::fromVariantMap(bsonObject.toMap());

    if (lastQueryType == GET_ROUTES) {
        m_routePoints = result["0"].toObject()["points"].toArray();

        emit routePointsChanged();

    } else {
        m_busModel->clear();
        QJsonArray busPoints = result["0"].toObject()["buses"].toArray();

        for (int i = 0; i < busPoints.size(); i++) {
            QList<QStandardItem *> items;
            QStandardItem *bus = new QStandardItem();
            bus->setData(QVariant(busPoints[i].toArray().at(0)), BusModel::Longitude);
            bus->setData(QVariant(busPoints[i].toArray().at(1)), BusModel::Latitude);
            items.append(bus);
            m_busModel->appendRow(items);
        }

        emit busModelChanged();
    }

}
