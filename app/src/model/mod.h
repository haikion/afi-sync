#ifndef MODITEM_H
#define MODITEM_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QList>
#include "apis/isync.h"
#include "syncitem.h"
#include "interfaces/imod.h"

#ifdef Q_OS_WIN
#pragma warning(push)
#pragma warning(disable: 4250)
#endif

class Repository;
class ModAdapter;

class Mod : public QObject, public SyncItem, virtual public IMod
{
    Q_OBJECT

public:
    Mod(const QString& name, const QString& key, ISync* sync);
    ~Mod();

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
    void appendModAdapter(ModAdapter* adapter);
    [[nodiscard]] bool optional() override;
    void stopUpdates();
    void startUpdates();
    void updateStatus();
    void forceCheck();
    [[nodiscard]] qint64 totalWanted() const;
    [[nodiscard]] qint64 totalWantedDone() const;
    [[nodiscard]] QString progressStr() override;
    [[nodiscard]] static QString bytesToMegasCeilStr(qint64 bytes);
    [[nodiscard]] static QString toProgressStr(qint64 totalWanted, qint64 totalWantedDone);
    [[nodiscard]] bool selected() override;

public slots:
    void repositoryChanged();
    void removeModAdapter(ModAdapter* modAdapter);
    void removeModAdapter(Repository* repository);

private:
    static const unsigned COMPLETION_WAIT_DURATION;

    bool moveFilesPostponed_{false};
    QString key_;
    ISync* sync_;
    QTimer* updateTimer_;
    unsigned waitTime_;
    QList<ModAdapter*> adapters_;
    QAtomicInteger<qint64> totalWanted_;
    QAtomicInteger<qint64> totalWantedDone_;
    QMutex repositoriesMutex_;
    std::atomic<bool> ticked_;
    QMutex adaptersMutex_;

    void buildPathHash();
    [[nodiscard]] bool reposInactive();
    void start();
    bool stop();
    void removeConflicting() const;
    [[nodiscard]] QString path() const;
    void setProcessCompletion(const bool value);
    [[nodiscard]] bool getProcessCompletion() const;
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
