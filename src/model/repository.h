#ifndef REPOSITORYITEM_H
#define REPOSITORYITEM_H

#include <QHash>
#include <QObject>
#include <QPair>
#include "syncitem.h"
#include "mod.h"
#include "rootitem.h"

typedef QHash<QString, QPair<QString, int>> PathHash;

class Repository : public SyncItem
{
    Q_OBJECT

public:
    Repository(const QString& name, const QString& serverAddress, unsigned port,
               QString password, RootItem* parent);
    ~Repository();

    virtual void appendMod(Mod* item);
    //void appendChild2(Mod* item);
    void update(TreeItem *item, int row = -1);
    //void updateProgress();
    virtual void checkboxClicked();
    void checkboxClicked(bool offline);
    BtsApi2* btsync() const;
    virtual QString startText();
    virtual QString joinText();
    void join();
    void launch();
    //void appendMod(const QString& name, const QString& key, bool optional = false);
    QList<Mod*> childItems() const;

//signals:
//    void enableChanged();
    void processCompletion();

    void enableMods();
private slots:
    void updateEtaAndStatus();
    void update();

private:
    BtsApi2* btsync_;
    //Server details
    QString serverAddress_;
    unsigned port_;
    QString password_;
    //Optimization: enables quick path->key search
    PathHash pathHash_;
    QTimer updateTimer_;
    //True if repo is ready
    bool ready_;

    void handleDirError(unsigned errorCode, BtsGetFoldersResult folder);
    QString modsParameter() const;
    QStringList joinParameters() const;
    void generalLaunch(const QStringList &extraParams = QStringList());
    void buildPathHash();
    int lastModified();
    RootItem* parentItem();
    QString createParFile(const QString& parameters);
};

#endif // REPOSITORYITEM_H
