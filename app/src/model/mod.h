#ifndef MODITEM_H
#define MODITEM_H

#include <QList>
#include <QObject>
#include <QSet>
#include <QTimer>

#include "apis/isync.h"
#include "interfaces/imod.h"
#include "syncitem.h"

#ifdef Q_OS_WIN
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

class ModAdapter;
class Repository;

class Mod : public QObject, public SyncItem, virtual public IMod
{
    Q_OBJECT

public:
    Mod(const QString& name, const QString& key, ISync* sync);
    ~Mod() override;

    // Moves files after mods download path has been changed
    void moveFiles();
    void check() override;
    [[nodiscard]] QString key() const;
    void checkboxClicked() override;
    [[nodiscard]] const QSet<Repository *> repositories() const;
    void deleteExtraFiles();
    bool ticked() override;
    void processCompletion() override;
    QList<ModAdapter*> modAdapters();
    [[nodiscard]] bool optional() override;
    void stopUpdates();
    void startUpdates();
    void updateStatus();
    [[nodiscard]] qint64 totalWanted() const;
    [[nodiscard]] qint64 totalWantedDone() const;
    [[nodiscard]] QString progressStr() override;
    [[nodiscard]] static QString bytesToMegasCeilStr(qint64 bytes);
    [[nodiscard]] static QString toProgressStr(qint64 totalWanted, qint64 totalWantedDone);
    [[nodiscard]] bool selected() override;

public slots:
    void appendModAdapter(ModAdapter* adapter);
    void forceCheck();
    void repositoryChanged();
    void removeModAdapter(ModAdapter* modAdapter);

private:
    static const unsigned COMPLETION_WAIT_DURATION;

    ISync* sync_;
    QAtomicInteger<qint64> totalWantedDone_{-1};
    QAtomicInteger<qint64> totalWanted_{-1};
    QList<ModAdapter*> adapters_;
    QMutex adaptersMutex_;
    QMutex repositoriesMutex_;
    QString key_;
    QTimer* updateTimer_{nullptr};
    bool moveFilesPostponed_{false};
    std::atomic<bool> ticked_{false};
    unsigned waitTime_{0};

    void buildPathHash();
    [[nodiscard]] bool reposInactive() const;
    void start();
    bool stop();
    void removeConflicting();
    [[nodiscard]] QString path();
    void setProcessCompletion(bool value);
    [[nodiscard]] bool getProcessCompletion();
    void updateProgress();
    void updateTicked();
    void moveFilesNow();

private slots:
    void update();
    void init();
    void threadConstructor();
    void onFolderAdded(const QString& key);
    void stopUpdatesSlot();
    void startUpdatesSlot();
    void moveFilesSlot();
    void checkboxClickedSlot();
};

#endif // MODITEM_H
