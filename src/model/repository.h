/*
 * Repository is a collection of mods which
 * are loaded into the game simultaniously.
 */

#ifndef REPOSITORYITEM_H
#define REPOSITORYITEM_H

#include <QHash>
#include <QObject>
#include <QPair>
#include "interfaces/irepository.h"
#include "syncitem.h"
#include "mod.h"

class Repository : public SyncItem, virtual public IRepository
{
public:
    Repository(const QString& name, const QString& serverAddress, unsigned port,
               QString password, ISync* sync);
    ~Repository();

    static Repository* findRepoByName(const QString& name, QList<Repository*> repositories);
    void check();
    void appendModAdapter(ModAdapter* adp, int index);
    void updateView(TreeItem* item, int row = -1);
    virtual void checkboxClicked();
    ISync* sync() const;
    virtual QString startText(); // TODO: Remove, QML
    virtual QString joinText(); // TODO: Remove, QML
    virtual void join();
    virtual void start();
    virtual bool optional() const;
    QList<Mod*> mods() const;
    QList<ISyncItem*> uiMods() const;
    void processCompletion();
    void enableMods();
    bool removeMod(const QString& key);
    bool removeMod(Mod* mod, bool removeFromSync = true);
    bool contains(const QString& key) const;
    void update();
    void startUpdates();
    void stopUpdates();
    void setBattlEyeEnabled(bool battlEyeEnabled);
    virtual bool ticked();
    virtual void setTicked(bool ticked);
    virtual QString progressStr();
    void setServerAddress(const QString& serverAddress);
    void setPort(const unsigned& port);
    void setPassword(const QString& password);    
    QSet<QString> modKeys() const;    
    void removeDeprecatedMods(const QSet<QString> jsonMods);
    void clearMods();

private:
    ISync* sync_;
    //Server details
    QString serverAddress_;
    unsigned port_;
    QString password_;
    //True if repo is ready
    bool ready_;
    bool battlEyeEnabled_;

    QString modsParameter();
    QStringList joinParameters() const;
    void generalLaunch(const QStringList& extraParams = QStringList());
    QString createParFile(const QString& parameters);
    void updateEtaAndStatus();
    void changed();
    QList<ModAdapter*> modAdapters() const;
    int calculateEta() const;
    static QSet<QString> createReadyStatuses();
};

#endif // REPOSITORYITEM_H
