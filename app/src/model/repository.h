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
#include <QSharedPointer>

#include "interfaces/irepository.h"
#include "mod.h"
#include "syncitem.h"

class Repository : public SyncItem, virtual public IRepository
{
public:
    Repository(const QString& name, const QString& serverAddress, unsigned port, const QString& password);
    ~Repository() override;

    [[nodiscard]] static QSharedPointer<Repository> findRepoByName(const QString& name, const QList<QSharedPointer<Repository>>& repositories);
    void check() override;
    void appendModAdapter(const QSharedPointer<ModAdapter>& adp, int index);
    void checkboxClicked() override;
    [[nodiscard]] ISync* sync() const;
    void join() override;
    void start() override;
    bool optional() override;
    [[nodiscard]] QList<QSharedPointer<Mod> > mods() const;
    [[nodiscard]] const QList<QSharedPointer<ModAdapter>>& modAdapters() const override;
    void processCompletion() override;
    bool removeMod(const QString& key);
    bool removeMod(Mod* mod, bool removeFromSync = true);
    [[nodiscard]] bool contains(const QString& key) const;
    void update();
    void startUpdates() const;
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
    [[nodiscard]] static QString createParFile(const QString& parameters);
    void changed() const;
    [[nodiscard]] static QSet<QString> createReadyStatuses();
    void removeAdapter(const QString& key);
};

#endif // REPOSITORYITEM_H
