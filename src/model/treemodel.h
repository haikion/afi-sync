#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QModelIndex>
#include <QVariant>
#include <QVariant>
#include <QObject>
#include <QList>
#include "repository.h"
#include "apis/isync.h"
#include "interfaces/ibandwidthmeter.h"

class RootItem;
class TreeItem;

class TreeModel : public QObject, virtual public IBandwidthMeter
{
    Q_OBJECT

public:
    // TODO: Remove QML Specific
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

    explicit TreeModel(QObject* parent = 0, ISync* sync = nullptr, bool haltGui = false);
    ~TreeModel();

    void updateView(TreeItem* item, int row = -1);
    void reset();
    void enableRepositories();
    void setHaltGui(bool halt);
    RootItem* rootItem() const;
    QList<IRepository*> repositories() const;

signals:
    void uploadChanged(QString newVal); // TODO: Remove QML specific
    void downloadChanged(QString newVal); // TODO: Remove QML specific

public slots:
    void rowsChanged(); // TODO: Remove QML specific
    QString downloadStr() const;
    QString uploadStr() const;
    void checkboxClicked(const QModelIndex& index);
    void launch(const QModelIndex& repoIdx) const;
    void join(const QModelIndex& repoIdx) const;
    void check(const QModelIndex& idx);
    QString versionString() const;
    void updateSpeed(qint64 downloadStr, qint64 uploadStr);
    bool ready(const QModelIndex& idx) const;

private:
    RootItem* rootItem_;
    unsigned download_;
    unsigned upload_;
    bool haltGui_;
    QList<Repository*> repositories_;

    void setupModelData(const QStringList& lines, TreeItem* parent);
    void postInit();
    QString bandwithString(int amount) const;
};

#endif // TREEMODEL_H
