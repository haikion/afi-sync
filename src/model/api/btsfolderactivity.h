#ifndef BTSFOLDERACTIVITY_H
#define BTSFOLDERACTIVITY_H
#include <QString>
#include <QList>
#include <QVariant>

//https://www.getsync.com/intl/en/api/docs#_folders__fid__activity_get
struct BtsFolderActivity
{
    int access;
    QString accessname;
    QString archive;
    int archive_files;
    int archive_size;
    bool canencrypt;
    int data_added;
    bool disabled;
    int down_eta;
    int down_speed;
    int down_status;
    QString encryptedsecret;
    int error;
    int files;
    bool has_key;
    QString hash;
    QString id;
    bool indexing;
    bool ismanaged;
    bool iswritable;
    int last_modfied;
    QString name;
    QString path;
    bool paused;
    QList<QVariant> peers; //TODO create peers
    QString readonlysecret;
    QString secret;
    QString secrettype;
    QString secrettypename;
    QString shareid;
    int size;
    QString status;
    bool stopped;
    int synclevel;
    int synclevelname;
    int up_eta;
    int up_speed;
    int up_status;
};

#endif // BTSFOLDERACTIVITY_H
