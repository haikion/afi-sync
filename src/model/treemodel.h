#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QModelIndex>
#include <QVariant>
#include <QVariant>
#include <QObject>
#include <QList>
#include <QSet>
#include "repository.h"
#include "apis/isync.h"
#include "interfaces/ibandwidthmeter.h"
#include "jsonreader.h"

class RootItem;
class TreeItem;

class TreeModel : public QObject, virtual public IBandwidthMeter
{
    Q_OBJECT

public:
    explicit TreeModel(QObject* parent = 0);
    ~TreeModel();

    void updateView(TreeItem* item, int row = -1); // TODO Remove, QML
    void reset();
    void enableRepositories();
    void setHaltGui(bool halt);
    RootItem* rootItem() const;
    QList<IRepository*> repositories() const; // TODO Remove, QML
    void moveFiles();
    void stopUpdates();
public slots:
    void rowsChanged(); // TODO: Remove QML specific
    QString downloadStr() const;
    QString uploadStr() const;
    void checkboxClicked(const QModelIndex& index);
    void launch(const QModelIndex& repoIdx) const;
    void join(const QModelIndex& repoIdx) const;
    void check(const QModelIndex& idx);
    QString versionString() const;
    void updateSpeed();
    bool ready(const QModelIndex& idx) const;

private slots:
    void update();
    void removeOrphans();

private:
    RootItem* rootItem_;
    unsigned download_;
    unsigned upload_;
    bool haltGui_;
    QList<Repository*> repositories_;
    QTimer updateTimer;
    ISync* sync_;

    void setupModelData(const QStringList& lines, TreeItem* parent);
    void postInit();
    QString bandwithString(int amount) const;
    QSet<Mod*> mods() const;
    void manageDeltaUpdates(const JsonReader& jsonReader);
    void createSync(const JsonReader& jsonReader);
};

#endif // TREEMODEL_H
