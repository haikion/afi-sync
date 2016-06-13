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
public:
    Repository(const QString& name, const QString& serverAddress, unsigned port,
               QString password, RootItem* parent);
    ~Repository();

    virtual void appendMod(Mod* item);
    void updateView(TreeItem* item, int row = -1);
    virtual void checkboxClicked();
    void checkboxClicked(bool offline);
    ISync* sync() const;
    virtual QString startText();
    virtual QString joinText();
    void join();
    void launch();
    QList<Mod*> mods() const;
    void processCompletion();
    void enableMods();
    bool removeMod(const QString& key);
    bool removeMod(Mod* mod);
    bool contains(const QString& key) const;
    void update();
    void startUpdates();
    void stopUpdates();

private:
    ISync* sync_;
    //Server details
    QString serverAddress_;
    unsigned port_;
    QString password_;
    //True if repo is ready
    bool ready_;

    QString modsParameter() const;
    QStringList joinParameters() const;
    void generalLaunch(const QStringList& extraParams = QStringList());
    RootItem* parentItem();
    QString createParFile(const QString& parameters);
    void updateEtaAndStatus();
    void changed(bool offline = false);
};

#endif // REPOSITORYITEM_H
