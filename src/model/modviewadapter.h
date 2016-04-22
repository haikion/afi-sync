#ifndef MODVIEWADAPTER_H
#define MODVIEWADAPTER_H

#include <QObject>
#include "syncitem.h"
#include "repository.h"
#include "mod.h"

/*
 * Solves the compatibility issue between tree structure and mod repo structure.
 * Mod can belong to multiple repositories, however TreeItem can only have one parent.
 * This item will have only one parent but mod_ might be shared amongst multiple ModViewAdapters.
 * Each repository has its own ModViewAdapter.
 */
class ModViewAdapter : public SyncItem
{
    Q_OBJECT

public:
    ModViewAdapter(Mod* mod, Repository* repo);

    virtual QString checkText();
    virtual QString startText();
    virtual QString joinText();
    virtual QString status() const;

    Mod* mod() const;

private:
    Mod* mod_;
    Repository* repo_;
    QTimer timer_;

private slots:
    void updateView();
};

#endif // MODVIEWADAPTER_H
