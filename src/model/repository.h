#ifndef REPOSITORYITEM_H
#define REPOSITORYITEM_H

#include <QHash>
#include <QObject>
#include <QPair>
#include "syncitem.h"
#include "mod.h"
#include "rootitem.h"

class Repository : public SyncItem
{
    Q_OBJECT

public:
    Repository(const QString& name, const QString& serverAddress, unsigned port,
               QString password, RootItem* parent);
    ~Repository();

    virtual void appendMod(Mod* item);
    void updateView(TreeItem *item, int row = -1);
    virtual void checkboxClicked();
    void checkboxClicked(bool offline);
    BtsApi2* btsync() const;
    virtual QString startText();
    virtual QString joinText();
    void join();
    void launch();
    QList<Mod*> mods() const;
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
    QTimer updateTimer_;
    //True if repo is ready
    bool ready_;

    void handleDirError(unsigned errorCode, BtsGetFoldersResult folder);
    QString modsParameter() const;
    QStringList joinParameters() const;
    void generalLaunch(const QStringList &extraParams = QStringList());
    int lastModified();
    RootItem* parentItem();
    QString createParFile(const QString& parameters);
};

#endif // REPOSITORYITEM_H
