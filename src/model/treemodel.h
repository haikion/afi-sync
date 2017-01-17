#ifndef TREEMODEL_H
#define TREEMODEL_H

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QVariant>
#include "apis/isync.h"

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
        Join = Qt::UserRole + 6,
        FileSize = Qt::UserRole + 7
    };

    explicit TreeModel(const QString& data, QObject* parent = 0);
    explicit TreeModel(unsigned port, QObject* parent = 0);
    TreeModel(QObject* parent = 0);
    ~TreeModel();

    QVariant data(const QModelIndex& index, int role) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex& index) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex& index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex& parent = QModelIndex()) const Q_DECL_OVERRIDE;
    void updateView(TreeItem* item, int row = -1);
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
    void reset();
    void enableRepositories();
    void setHaltGui(bool halt);
    RootItem* rootItem() const;

signals:
    void uploadChanged(QString newVal);
    void downloadChanged(QString newVal);

public slots:
    void rowsChanged();
    QString getDownload() const;
    QString getUpload() const;
    bool isRepository(const QModelIndex& index) const;
    void checkboxClicked(const QModelIndex& index);
    void launch(const QModelIndex& repoIdx) const;
    void join(const QModelIndex& repoIdx) const;
    void resetSync();
    void processCompletion();
    QString versionString() const;
    void updateSpeed(qint64 download, qint64 upload);

private:
    RootItem* rootItem_;
    unsigned download_;
    unsigned upload_;
    bool haltGui_;

    void setupModelData(const QStringList& lines, TreeItem* parent);
    void postInit();
    QString bandwithString(int amount) const;
};
//! [0]

#endif // TREEMODEL_H
