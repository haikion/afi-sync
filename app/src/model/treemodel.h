#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QList>
#include <QObject>
#include <QSet>
#include <QSharedPointer>
#include <QTimer>

#include "interfaces/ibandwidthmeter.h"
#include "jsonreader.h"
#include "versioncheckresult.h"

class IRepository;
class Mod;
class Repository;

class TreeModel : public QObject, virtual public IBandwidthMeter
{
    Q_OBJECT

public:
    explicit TreeModel(QObject* parent = nullptr);
    ~TreeModel() override;

    VersionCheckResult checkVersion() const;
    void reset();
    void enableRepositories();
    void setHaltGui(bool halt);
    [[nodiscard]] QList<IRepository*> repositories() const;
    void moveFiles();
    void stopUpdates();
    [[nodiscard]] QString downloadStr() const override;
    [[nodiscard]] QString uploadStr() const override;

public slots:
    void updateSpeed();

signals:
    void repositoriesChanged(const QList<IRepository*>& repositories);

private slots:
    void update();
    void periodicRepoUpdate();

private:
    int64_t download_{0};
    int64_t upload_{0};
    RepositoryList repositories_;
    QTimer updateTimer_;
    LibTorrentApi* libTorrentApi_{nullptr};
    QTimer repoUpdateTimer_;
    JsonReader jsonReader_;
    bool mirroringDeltaPatches_{false};

    void postInit();
    [[nodiscard]] static QString bandwithString(int64_t amount);
    [[nodiscard]] const QSet<Mod*> mods() const;
    void createSync(const QStringList& deltaUrls);
    void updateRepositories();
    static QList<IRepository*> toIrepositories(const RepositoryList& repositories);
};

#endif // TREEMODEL_H
