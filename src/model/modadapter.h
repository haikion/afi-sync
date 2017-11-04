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
    Q_OBJECT

public:
    ModAdapter(Mod* mod, Repository* repo, bool isOptional, int index);
    ~ModAdapter() = default;

    virtual QString checkText();
    virtual QString startText();
    virtual QString joinText();
    virtual QString status() const;
    virtual QString progressText();
    virtual void checkboxClicked();
    virtual bool ticked() const;
    virtual void processCompletion();
    virtual int eta() const;
    bool isOptional() const;
    Mod* mod() const;
    Repository* repo() const;

protected:
    virtual void setTicked(bool ticked);

public slots:
    void updateView();

private:
    Mod* mod_;
    Repository* repo_;
    bool isOptional_;
    QString guiData_;
};

#endif // MODVIEWADAPTER_H
