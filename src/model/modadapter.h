#ifndef MODVIEWADAPTER_H
#define MODVIEWADAPTER_H

#include "syncitem.h"
#include "repository.h"
#include "mod.h"

/*
 * Solves the compatibility issue between tree structure and mod repo structure.
 * Mod can belong to multiple repositories, however TreeItem can only have one parent.
 * This item will have only one parent but mod_ might be shared amongst multiple ModAdapter.
 * Each repository has its own ModAdapter. Also solves the problem in which mod is in two repositories
 * but optional in only one.
 */
class ModAdapter : public SyncItem
{
public:
    ModAdapter(Mod* mod, Repository* repo, bool isOptional);
    ~ModAdapter();

    virtual QString checkText();
    virtual QString startText();
    virtual QString joinText();
    virtual QString status() const;
    virtual void checkboxClicked();
    virtual bool ticked() const;
    void setTicked(bool ticked);
    bool isOptional() const;

    Mod* mod() const;
    void updateView();

private:
    Mod* mod_;
    Repository* repo_;
    QString tickedKey_;
    bool isOptional_;
};

#endif // MODVIEWADAPTER_H
