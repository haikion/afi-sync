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

    [[nodiscard]] static Repository* findRepoByName(const QString& name, const QList<Repository*>& repositories);
    void check() override;
    void appendModAdapter(QSharedPointer<ModAdapter> adp, int index);
    void checkboxClicked() override;
    [[nodiscard]] ISync* sync() const;
    void join() override;
    void start() override;
    bool optional() override;
    [[nodiscard]] QList<QSharedPointer<Mod> > mods() const;
    [[nodiscard]] QList<IMod*> uiMods() const override;
    void processCompletion() override;
    void enableMods();
    bool removeMod(const QString& key);
    bool removeMod(Mod* mod, bool removeFromSync = true);
    [[nodiscard]] bool contains(const QString& key) const;
    void update();
    void startUpdates();
    void stopUpdates();
    void setBattlEyeEnabled(bool battlEyeEnabled);
    [[nodiscard]] bool isReady() const;
    bool ticked() override;
    virtual void setTicked(bool ticked);
    QString progressStr() override;
    void setServerAddress(const QString& serverAddress);
    void setPort(unsigned port);
    void setPassword(const QString& password);    
    [[nodiscard]] const QSet<QString> modKeys() const;
    void removeDeprecatedMods(const QSet<QString>& jsonMods);
    void clearModAdapters();

private:
    //Server details
    QString serverAddress_;
    unsigned port_;
    QString password_;
    bool battlEyeEnabled_;
    // TODO: Create IModAdapter and use that instead so Repository
    // can be unit tested
    QList<QSharedPointer<ModAdapter>> modAdapters_;
    QElapsedTimer activeTimer_;

    [[nodiscard]] QString modsParameter();
    [[nodiscard]] QStringList joinParameters() const;
    void generalLaunch(const QStringList& extraParams = QStringList());
    [[nodiscard]] QString createParFile(const QString& parameters);
    void changed();
    [[nodiscard]] QList<QSharedPointer<ModAdapter>> modAdapters() const;
    [[nodiscard]] static QSet<QString> createReadyStatuses();
    void removeAdapter(const QString& key);
};

#endif // REPOSITORYITEM_H
