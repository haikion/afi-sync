#ifndef MODITEM_H
#define MODITEM_H

#include <QThread>
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
    Mod(const QString& name, const QString& key);
    ~Mod();

    QString key() const;
    virtual Repository* parentItem();
    //void handleDirError(BtsFolderActivity folder);
    void appendRepository(Repository* repository);
    virtual void checkboxClicked();
    virtual QString checkText();
    virtual QString startText();
    virtual QString joinText();
    QSet<Repository*> repositories() const;
    //int lastModified();
    void deleteExtraFiles();
    virtual bool ticked() const;
    void processCompletion();
    QVector<ModAdapter*> modAdapters() const;
    void appendModAdapter(ModAdapter* adapter);
    void stopUpdates();
    void startUpdates();
    void updateStatus();
    bool isOptional() const;

public slots:
    void repositoryChanged(bool offline = false);
    void threadDestructor();
    bool removeRepository(Repository* repository);

private:
    static const unsigned COMPLETION_WAIT_DURATION;

    QString key_;
    ISync* sync_;
    QTimer* updateTimer_;
    QSet<Repository*> repositories_;
    unsigned waitTime_;
    QVector<ModAdapter*> adapters_;

    void buildPathHash();
    void fetchEta();
    bool reposInactive() const;
    void start();
    bool stop();

private slots:
    void update();
    void init();
};

#endif // MODITEM_H
