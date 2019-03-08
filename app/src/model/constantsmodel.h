/*
 * Combatilibity from QML to Constants namespace
 */
#ifndef CONSTANTSMODEL_H
#define CONSTANTSMODEL_H

#include <QObject>

class ConstantsModel: public QObject
{
    Q_OBJECT
public:
    ConstantsModel(QObject* parent = nullptr);

public slots:
    static QString defaultPort();
};

#endif // CONSTANTSMODEL_H
