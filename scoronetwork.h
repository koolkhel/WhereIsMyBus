#ifndef SCORONETWORK_H
#define SCORONETWORK_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>

#include <QStandardItemModel>

// the backend
class ScoroNetwork : public QObject
{
    Q_OBJECT
public:
    explicit ScoroNetwork(QObject *parent = 0);
    virtual ~ScoroNetwork();
    Q_PROPERTY(QString routeName READ routeName WRITE setRouteName NOTIFY routeNameChanged)
    Q_PROPERTY(QJsonArray routePoints READ routePoints NOTIFY routePointsChanged)
    Q_PROPERTY(QStandardItemModel* busModel READ busModel NOTIFY busModelChanged)
signals:
    void routeNameChanged();
    void routePointsChanged();
    void busModelChanged();
public slots:
    void getRoutesAndPoints();

    QStandardItemModel* busModel() {
        return m_busModel;
    }

    QJsonArray routePoints() {
        return m_routePoints;
    }

    QString routeName() {
        return m_routeName;
    }

    void setRouteName(QString _routeName) {
        m_routeName = _routeName;
        emit routeNameChanged();
    }

    void updateTimerTick();

private slots:
    void replyFinished(QNetworkReply *reply);
private:

    QNetworkAccessManager *manager;
    QNetworkReply *currentReply;
    QTimer *updateTimer;

    QString m_routeName;
    QJsonArray m_routePoints;
    QStandardItemModel *m_busModel;

    enum LastQueryType {
        NOTHING,
        GET_ROUTES,
        GET_BUSES
    } lastQueryType;

    QString apiUrl;
};

#endif // SCORONETWORK_H
