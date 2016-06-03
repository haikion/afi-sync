#ifndef ROOTITEM_H
#define ROOTITEM_H

#include <QObject>
#include <QTimer>
#include "apis/isync.h"
#include "treemodel.h"
#include "treeitem.h"

class Repository;

class RootItem : public QObject, public TreeItem
{
    Q_OBJECT

public:
    explicit RootItem(TreeModel* parentModel);
    explicit RootItem(const QString& username, const QString& password,
                      unsigned port, TreeModel* parentModel);
    virtual ~RootItem();

    void updateView(TreeItem* item, int row = -1);
    ISync* sync() const;
    QList<Repository*> childItems();
    void resetSyncSettings();

    void enableRepositories();
    void processCompletion();

private slots:
    void removeOrphans();
    void updateSpeed();

private:
    bool initializing_;
    TreeModel* parent_;
    ISync* sync_;
    QTimer updateTimer_;
    QString username_;
    QString password_;
    unsigned port_;

    void initSync();
    QString defaultter(const QString& value, const QString& defaultValue);
};

#endif // ROOTITEM_H
