#ifndef ROOTITEM_H
#define ROOTITEM_H

#include <QObject>
#include <QTimer>
#include "apis/isync.h"
#include "treemodel.h"
#include "treeitem.h"
#include "jsonreader.h"

class Repository;
class JsonReader;

class RootItem : public QObject, public TreeItem
{
    Q_OBJECT

public:
    explicit RootItem(TreeModel* parentModel);
    virtual ~RootItem();

    void updateView(TreeItem* item, int row = -1);
    ISync* sync() const;
    QList<Repository*> childItems() const;
    void resetSyncSettings();

    void enableRepositories();
    void processCompletion();
    void rowsChanged();
    bool stopUpdates();
    void startUpdates();
    void startUpdateTimers();

private slots:
    void removeOrphans();
    void updateSpeed();
    void periodicRepoUpdate();
    void update();

private:
    static const int REPO_UPDATE_DELAY;

    bool initializing_;
    TreeModel* parent_;
    ISync* sync_;
    QTimer updateTimer_;
    QTimer repoTimer_;
    JsonReader jsonReader_;

    void initSync();
    QString defaultter(const QString& value, const QString& defaultValue);
};

#endif // ROOTITEM_H
