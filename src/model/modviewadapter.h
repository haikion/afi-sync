#ifndef MODVIEWADAPTER_H
#define MODVIEWADAPTER_H

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
public:
    ModViewAdapter(Mod* mod, Repository* repo);
    ~ModViewAdapter();

    virtual QString checkText();
    virtual QString startText();
    virtual QString joinText();
    virtual QString status() const;
    virtual void checkboxClicked();

    Mod* mod() const;
    void updateView();

private:
    Mod* mod_;
    Repository* repo_;
};

#endif // MODVIEWADAPTER_H
