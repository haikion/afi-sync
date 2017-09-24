#ifndef MODITEM_H
#define MODITEM_H

#include <QObject>
#include <QTimer>
#include <QSet>
#include <QVector>
#include "apis/isync.h"
#include "syncitem.h"
#include "bissignaturechecker.h"


class Repository;
class ModAdapter;

class Mod : public SyncItem
{
    Q_OBJECT

public:
    Mod(const QString& name, const QString& key, BisSignatureChecker* checker);
    ~Mod();

    QString key() const;
    virtual Repository* parentItem();
    void appendRepository(Repository* repository);
    virtual void checkboxClicked();
    virtual QString startText();
    virtual QString joinText();
    QSet<Repository*> repositories() const;
    void deleteExtraFiles();
    virtual bool ticked() const;
    QVector<ModAdapter*> modAdapters() const;
    void appendModAdapter(ModAdapter* adapter);
    void stopUpdates();
    void startUpdates();
    void updateStatus();

public slots:
    void repositoryChanged(bool offline = false);
    bool removeRepository(Repository* repository);
    void processCompletion();

private:
    static const unsigned COMPLETION_WAIT_DURATION;

    QString key_;
    ISync* sync_;
    QTimer* updateTimer_;
    QSet<Repository*> repositories_;
    unsigned waitTime_;
    QVector<ModAdapter*> adapters_;
    BisSignatureChecker* checker_;

    void buildPathHash();
    void updateEta();
    void updateView(bool force = false);
    bool reposInactive() const;
    void start();
    bool stop();
    void removeConflicting() const;
    QString path() const;
    bool isOptional() const;
    void setProcessCompletion(bool value);
    bool getProcessCompletion() const;
    bool folderChecking() const;

private slots:
    void update(bool force = false);
    void init();
    void threadConstructor();
    void stopUpdatesSlot();
    void handleBisCheckDone(SigCheckProcess::CheckStatus status);
};

#endif // MODITEM_H
