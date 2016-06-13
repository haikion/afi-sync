#ifndef MODITEM_H
#define MODITEM_H

#include <vector>
#include <QThread>
#include <QObject>
#include <QTimer>
#include <QSet>
#include <QVector>
#include "apis/isync.h"
#include "syncitem.h"

class Repository;
class ModViewAdapter;

class Mod : public SyncItem
{
    Q_OBJECT

public:
    Mod(const QString& name, const QString& key, bool isOptional);
    ~Mod();

    QString key() const;
    virtual Repository* parentItem();
    //void handleDirError(BtsFolderActivity folder);
    void addRepository(Repository* repository);
    virtual void checkboxClicked();
    virtual QString checkText();
    virtual QString startText();
    virtual QString joinText();
    QSet<Repository*> repositories() const;
    //int lastModified();
    void deleteExtraFiles();
    virtual bool checked() const;
    void processCompletion();
    QVector<ModViewAdapter*> viewAdapters() const;
    void addModViewAdapter(ModViewAdapter* adapter);
    void stopUpdates();
    void startUpdates();
    void updateStatus();

public slots:
    void repositoryChanged(bool offline = false);
    void threadDestructor();
    bool removeRepository(Repository* repository);

private:
    static const unsigned COMPLETION_WAIT_DURATION;

    bool isOptional_;
    QString key_;
    ISync* sync_;
    QTimer* updateTimer_;
    QSet<Repository*> repositories_;
    unsigned waitTime_;
    QVector<ModViewAdapter*> viewAdapters_;

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
