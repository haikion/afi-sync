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
class ModAdapter : public SyncItem, virtual public IMod
{
public:
    ModAdapter(Mod* mod, Repository* repo, bool optional, int index);
    ~ModAdapter() = default;

    virtual void check();
    virtual QString checkText();
    virtual QString statusStr();
    virtual QString etaStr() const;
    virtual void checkboxClicked();
    virtual bool ticked();
    virtual void processCompletion();
    virtual int eta() const;
    bool optional() override;
    virtual QString progressStr();
    Mod* mod() const;
    Repository* repo() const;
    void forceCheck() const;
    bool selected() override;

protected:
    virtual void setTicked(bool ticked);

private:
    Mod* mod_;
    Repository* repo_;
    bool isOptional_;
    QString guiData_;
};

#endif // MODVIEWADAPTER_H