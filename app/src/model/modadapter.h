#ifndef MODVIEWADAPTER_H
#define MODVIEWADAPTER_H

#include "repository.h"
#include "settingsmodel.h"
#include "syncitem.h"
#include "interfaces/imod.h"

class Mod;

/*
 * Solves the compatibility issue between tree structure and mod repo structure.
 * Mod can belong to multiple repositories, however TreeItem can only have one parent.
 * ModAdapter will belong to only one repository but mod can be owned by multiple ModAdapters.
 * It also solves the problem in which mod is in two repositories but optional in only one.
 */
class ModAdapter : public SyncItem, virtual public IMod
{
    Q_OBJECT

public:
    ModAdapter(const QSharedPointer<Mod>& mod, Repository* repo, bool optional, int index);
    ~ModAdapter() override;

    void check() override;
    [[nodiscard]] QString statusStr() override;
    void checkboxClicked() override;
    [[nodiscard]] bool ticked() override;
    void processCompletion() override;
    [[nodiscard]] bool optional() override;
    [[nodiscard]] QString progressStr() override;
    /**
     * @brief path
     * @return Sanitized directory path for the mod
     */
    [[nodiscard]] QString path() const;
    [[nodiscard]] QSharedPointer<Mod> mod() const;
    [[nodiscard]] Repository* repo() const;
    void forceCheck() const;
    bool selected() override;
    [[nodiscard]] QString key() const;
    void stopUpdates();

signals:
    void fileSizeInitialized();

protected:
    virtual void setTicked(bool ticked);

private:
    QSharedPointer<Mod> mod_{nullptr};
    QString key_;
    Repository* repo_{nullptr};
    SettingsModel& settings_{SettingsModel::instance()};
    std::atomic<bool> isOptional_;
};

#endif // MODVIEWADAPTER_H
