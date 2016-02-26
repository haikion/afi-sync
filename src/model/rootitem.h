#ifndef ROOTITEM_H
#define ROOTITEM_H

#include <QObject>
#include <QTimer>
#include "apis/btsync/btsapi2.h"
#include "treemodel.h"
#include "treeitem.h"

class Repository;

class RootItem : public TreeItem
{

public:
    explicit RootItem(TreeModel* parentModel);
    explicit RootItem(const QString& username, const QString& password,
                      unsigned port, TreeModel* parentModel);
    virtual ~RootItem();

    void updateView(TreeItem* item, int row = -1);
    BtsApi2* btsync() const;
    QList<Repository*> childItems();
    void resetSyncSettings();

    void enableRepositories();
    void processCompletion();
private:
    bool initializing_;
    TreeModel* parent_;
    BtsApi2* sync_;
    QTimer updateTimer_;
    QString username_;
    QString password_;
    unsigned port_;

    void initSync();
    void removeOrphans();
    QString defaultter(const QString& value, const QString& defaultValue);
};

#endif // ROOTITEM_H
