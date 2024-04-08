/*
 * Repository is a collection of mods which
 * are loaded into the game simultaniously.
 */

#ifndef REPOSITORYITEM_H
#define REPOSITORYITEM_H

#include <QElapsedTimer>
#include <QHash>
#include <QObject>
#include <QPair>
#include "interfaces/irepository.h"
#include "syncitem.h"
#include "mod.h"

class Repository : public SyncItem, virtual public IRepository
{
public:
    Repository(const QString& name, const QString& serverAddress, unsigned port, const QString& password);
    ~Repository() override;

    static Repository* findRepoByName(const QString& name, QList<Repository*> repositories);
    void check() override;
    void appendModAdapter(ModAdapter* adp, int index);
    void checkboxClicked() override;
    ISync* sync() const;
    void join() override;
    void start() override;
    bool optional() override;
    QList<Mod*> mods() const;
    QList<IMod*> uiMods() const override;
    void processCompletion() override;
    void enableMods();
    bool removeMod(const QString& key);
    bool removeMod(Mod* mod, bool removeFromSync = true);
    bool contains(const QString& key) const;
    void update();
    void startUpdates();
    void stopUpdates();
    void setBattlEyeEnabled(bool battlEyeEnabled);
    bool isReady() const;
    bool ticked() override;
    virtual void setTicked(bool ticked);
    QString progressStr() override;
    void setServerAddress(const QString& serverAddress);
    void setPort(const unsigned& port);
    void setPassword(const QString& password);    
    QSet<QString> modKeys() const;    
    void removeDeprecatedMods(const QSet<QString>& jsonMods);
    void clearModAdapters();
    void removeModAdapter(ModAdapter* modAdapter);

private:
    //Server details
    QString serverAddress_;
    unsigned port_;
    QString password_;
    bool battlEyeEnabled_;
    // TODO: Create IModAdapter and use that instead so Repository
    // can be unit tested
    QList<ModAdapter*> modAdapters_;
    QElapsedTimer activeTimer_;

    QString modsParameter();
    QStringList joinParameters() const;
    void generalLaunch(const QStringList& extraParams = QStringList());
    QString createParFile(const QString& parameters);
    void changed();
    QList<ModAdapter*> modAdapters() const;
    static QSet<QString> createReadyStatuses();
    void removeAdapter(const Mod* mod);
    void removeAdapter(const QString& key);
};

#endif // REPOSITORYITEM_H
