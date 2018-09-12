#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QVariant>
#include "apis/isync.h"
#include "interfaces/ibandwidthmeter.h"

class RootItem;
class TreeItem;

//! [0]
class TreeModel : public QAbstractItemModel, public IBandwidthMeter
{
    Q_OBJECT
    Q_PROPERTY(QString download READ downloadStr NOTIFY downloadChanged)
    Q_PROPERTY(QString upload READ uploadStr NOTIFY uploadChanged)

public:
    enum  {
        Check = Qt::UserRole + 1,
        Name = Qt::UserRole + 2,
        Status = Qt::UserRole + 3,
        Progress = Qt::UserRole + 4,
        Start = Qt::UserRole + 5,
        Join = Qt::UserRole + 6,
        FileSize = Qt::UserRole + 7
    };
    static const QHash<int, QByteArray> ROLE_NAMES;

    explicit TreeModel(QObject* parent = 0, bool haltGui = false);
    ~TreeModel();

    QVariant data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void updateView(TreeItem* item, int row = -1);
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
    void reset();
    void enableRepositories();
    void setHaltGui(bool halt);
    RootItem* rootItem() const;

signals:
    void uploadChanged(QString newVal);
    void downloadChanged(QString newVal);

public slots:
    void rowsChanged();
    QString downloadStr();
    QString uploadStr();
    bool isRepository(const QModelIndex& index) const;
    void checkboxClicked(const QModelIndex& index);
    void launch(const QModelIndex& repoIdx) const;
    void join(const QModelIndex& repoIdx) const;
    void resetSync();
    void processCompletion();
    void check(const QModelIndex& idx);
    QString versionString() const;
    void updateSpeed(qint64 downloadStr, qint64 uploadStr);
    bool ready(const QModelIndex& idx) const;

private:
    RootItem* rootItem_;
    unsigned download_;
    unsigned upload_;
    bool haltGui_;

    void setupModelData(const QStringList& lines, TreeItem* parent);
    void postInit();
    QString bandwithString(int amount) const;
};
//! [0]

#endif // TREEMODEL_H
