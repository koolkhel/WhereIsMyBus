#ifndef BUSMODEL_H
#define BUSMODEL_H


#include <QStandardItemModel>

class BusModel : public QStandardItemModel
{
    Q_OBJECT
public:

    enum Role {
        Latitude=Qt::UserRole,
        Longitude
    };


    explicit BusModel(QObject * parent = 0): QStandardItemModel(parent)
    {
    }

    explicit BusModel( int rows, int columns, QObject * parent = 0 ): QStandardItemModel(rows, columns, parent)
    {
    }

    QHash<int, QByteArray> roleNames() const
    {
         QHash<int, QByteArray> roles;
         roles[Latitude] = "latitude";
         roles[Longitude] = "longitude";

         return roles;
    }
};
#endif // BUSMODEL_H
