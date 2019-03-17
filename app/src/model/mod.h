#ifndef MODITEM_H
#define MODITEM_H

#include <memory>
#include <QObject>
#include <QTimer>
#include <QSet>
#include <QList>
#include "apis/isync.h"
#include "syncitem.h"
#include "interfaces/imod.h"

class Repository;
class ModAdapter;

class Mod : public QObject, public SyncItem, virtual public IMod
{
    Q_OBJECT

public:
    Mod(const QString& name, const QString& key, std::shared_ptr<ISync> sync);
    ~Mod();

    // Moves files after mods download path has been changed
    void moveFiles();
    void check() override;
    QString key() const;
    void checkboxClicked() override;
    QSet<Repository*> repositories() const;
    void deleteExtraFiles();
    bool ticked() override;
    void processCompletion() override;
    QList<ModAdapter*> modAdapters();
    void appendModAdapter(ModAdapter* adapter);
    bool optional() override;
    void stopUpdates();
    void startUpdates();
    void updateStatus();
    void forceCheck();
    qint64 totalWanted() const;
    qint64 totalWantedDone() const;
    QString progressStr() override;
    static QString bytesToMegasCeilStr(const qint64 bytes);
    static QString toProgressStr(const qint64 totalWanted, qint64 totalWantedDone);
    bool selected() override;

public slots:
    void repositoryChanged();
    void removeModAdapter(ModAdapter* modAdapter);
    void removeModAdapter(Repository* repository);

private:
    static const unsigned COMPLETION_WAIT_DURATION;

    QString key_;
    std::shared_ptr<ISync> sync_;
    QTimer* updateTimer_;
    unsigned waitTime_;
    QList<ModAdapter*> adapters_;
    QAtomicInteger<qint64> totalWanted_;
    QAtomicInteger<qint64> totalWantedDone_;
    QMutex repositoriesMutex_;
    std::atomic<bool> ticked_;
    QMutex adaptersMutex_;

    void buildPathHash();
    bool reposInactive();
    void start();
    bool stop();
    void removeConflicting() const;
    QString path() const;
    void setProcessCompletion(const bool value);
    bool getProcessCompletion() const;
    void updateProgress();
    void updateTicked();

private slots:
    void update();
    void init();
    void threadConstructor();
    void stopUpdatesSlot();
    void startUpdatesSlot();
    void moveFilesSlot();
    void checkboxClickedSlot();
};

#endif // MODITEM_H
