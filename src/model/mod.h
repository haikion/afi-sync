#ifndef MODITEM_H
#define MODITEM_H

#include <vector>
#include <QObject>
#include "api/btsapi2.h"
#include "syncitem.h"

class Repository;

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
    void removeRepository(Repository* repository);
    virtual void checkboxClicked();
    virtual QString checkText();
    virtual QString startText();
    virtual QString joinText();
    std::vector<Repository*> repositories() const;
    int lastModified();
    void deleteExtraFiles();
    virtual bool checked() const;

public slots:
    void repositoryEnableChanged(bool offline = false);
    void init();

private:
    bool isOptional_;
    QString key_;
    BtsApi2* btsync_;
    QTimer updateTimer_;
    //QList<Repository*> repositories_;
    std::vector<Repository*> repositories_;

private slots:
    void fetchEta();
    void updateStatus();
    void updateView();
    void start();

};

#endif // MODITEM_H
