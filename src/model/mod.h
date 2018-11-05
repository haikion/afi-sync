#ifndef MODITEM_H
#define MODITEM_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QVector>
#include "apis/isync.h"
#include "syncitem.h"

class Repository;
class ModAdapter;

class Mod : public SyncItem
{
    Q_OBJECT

public:
    Mod(const QString& name, const QString& key, ISync* sync);
    ~Mod();

    // Moves files after mods download path has been changed
    void moveFiles();
    void check();
    QString key() const;
    virtual Repository* parentItem();
    void appendRepository(Repository* repository);
    virtual void checkboxClicked();
    virtual QString startText();
    virtual QString joinText();
    QSet<Repository*> repositories() const;
    void deleteExtraFiles();
    virtual bool ticked() const;
    void processCompletion();
    QVector<ModAdapter*> modAdapters() const;
    void appendModAdapter(ModAdapter* adapter);
    virtual bool optional() const;
    void stopUpdates();
    void startUpdates();
    void updateStatus();
    void forceCheck();
    qint64 totalWanted() const;
    qint64 totalWantedDone() const;
    virtual QString progressStr() const;
    static QString bytesToMegasStr(const qint64 bytes);
    static QString toProgressStr(const qint64 totalWanted, const qint64 totalWantedDone);

public slots:
    void repositoryChanged(bool offline = false);
    bool removeRepository(Repository* repository);

private:
    static const unsigned COMPLETION_WAIT_DURATION;

    QString key_;
    ISync* sync_;
    QTimer* updateTimer_;
    QSet<Repository*> repositories_;
    unsigned waitTime_;
    QVector<ModAdapter*> adapters_;
    QAtomicInteger<qint64> totalWantedDone_;
    QAtomicInteger<qint64> totalWanted_;

    void buildPathHash();
    void updateEta();
    bool reposInactive() const;
    void start();
    bool stop();
    void removeConflicting() const;
    QString path() const;
    void setProcessCompletion(bool value);
    bool getProcessCompletion() const;
    void updateProgress();
private slots:
    void update();
    void init();
    void threadConstructor();
    void stopUpdatesSlot();
};

#endif // MODITEM_H
