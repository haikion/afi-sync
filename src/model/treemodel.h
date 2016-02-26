#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QVariant>
#include "apis/btsync/libbtsync-qt/bts_spawnclient.h"
#include "apis/btsync/btsapi2.h"

class RootItem;
class TreeItem;

//! [0]
class TreeModel : public QAbstractItemModel
{
    Q_OBJECT
    Q_PROPERTY(QString download READ getDownload NOTIFY downloadChanged)
    Q_PROPERTY(QString upload READ getUpload NOTIFY uploadChanged)

public:
    enum  {
        Check = Qt::UserRole + 1,
        Name = Qt::UserRole + 2,
        Status = Qt::UserRole + 3,
        Progress = Qt::UserRole + 4,
        Start = Qt::UserRole + 5,
        Join = Qt::UserRole + 6
    };

    explicit TreeModel(const QString &data, QObject *parent = 0);
    explicit TreeModel(const QString& username, const QString& password, unsigned port, QObject* parent = 0);
    TreeModel(QObject* parent = 0);
    ~TreeModel();

    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    //QVariant headerData(int section, Qt::Orientation orientation,
    //                    int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void updateView(TreeItem *item, int row = -1);
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
    void reset();

    void enableRepositories();
signals:
    void uploadChanged(QString newVal);
    void downloadChanged(QString newVal);

public slots:
    QString getDownload() const;
    QString getUpload() const;
    bool isRepository(const QModelIndex& index) const;
    void checkboxClicked(const QModelIndex& index);
    void launch(const QModelIndex& repoIdx) const;
    void join(const QModelIndex& repoIdx) const;
    void resetSync();
    void processCompletion();
    //Hack to kill zombie. If signal is received on time
    //program won't kill itself. Signal is not produced by zombie.
    void setDieFalse();
    void die();
    void killItWithFire(); //Force kill just in case...

private slots:
    void updateSpeed(qint64 download, qint64 upload);

private:
    RootItem* rootItem_;
    unsigned download_;
    unsigned upload_;
    bool die_;

    void setupModelData(const QStringList &lines, TreeItem *parent);
    void setClient(BtsClient* newclient);
    void initBtsync();
    BtsClient*createBtsClient();
    void handleZombie(BtsSpawnClient* btsclient);
    void postInit();
    QString bandwithString(int amount) const;
};
//! [0]

#endif // TREEMODEL_H
