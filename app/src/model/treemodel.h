#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QVariant>
#include <QVariant>
#include <QObject>
#include <QList>
#include <QSet>
#include "repository.h"
#include "apis/isync.h"
#include "interfaces/ibandwidthmeter.h"
#include "jsonreader.h"

class TreeModel : public QObject, virtual public IBandwidthMeter
{
    Q_OBJECT

public:
    explicit TreeModel(QObject* parent = nullptr);
    ~TreeModel() override;

    void reset();
    void enableRepositories();
    void setHaltGui(bool halt);
    QList<IRepository*> repositories() const;
    void moveFiles();
    void stopUpdates();
    QString downloadStr() const override;
    QString uploadStr() const override;

public slots:
    void updateSpeed();

signals:
    void repositoriesChanged(QList<IRepository*> repositories);

private slots:
    void update();
    void periodicRepoUpdate();

private:
    unsigned download_;
    unsigned upload_;
    QList<Repository*> repositories_;
    QTimer updateTimer_;
    ISync* sync_;
    QTimer repoUpdateTimer_;
    JsonReader jsonReader_;
    bool mirroringDeltaPatches_{false};

    void postInit();
    QString bandwithString(int amount) const;
    const QSet<Mod*> mods() const;
    void createSync(const QStringList& deltaUrls);
    void updateRepositories();
    static QList<IRepository*> toIrepositories(const QList<Repository*>& repositories);
};

#endif // TREEMODEL_H
