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
#include "settingsmodel.h"
#include "syncitem.h"

class Mod;

class Repository : public SyncItem, virtual public IRepository
{
    Q_OBJECT

public:
    Repository(const QString& name, const QString& serverAddress, unsigned port, const QString& password);
    ~Repository() override;

    void check() override;
    void appendModAdapter(const QSharedPointer<ModAdapter>& adp, int index);
    void checkboxClicked() override;
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
    QList<QSharedPointer<ModAdapter>> modAdapters_;
    QElapsedTimer activeTimer_;
    SettingsModel& settings_{SettingsModel::instance()};

    [[nodiscard]] QSet<QString> getModStatuses() const;
    [[nodiscard]] QString modsParameter();
    [[nodiscard]] QStringList joinParameters() const;
    void generalLaunch(const QStringList& extraParams = QStringList());
    [[nodiscard]] static QString createParFile(const QString& parameters);
    void changed() const;
    [[nodiscard]] static QSet<QString> createReadyStatuses();
    void removeAdapter(const QString& key);
};

#endif // REPOSITORYITEM_H
