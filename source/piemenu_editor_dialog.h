#ifndef PIEMENU_EDITOR_DIALOG_H__
#define PIEMENU_EDITOR_DIALOG_H__

#include "c4d.h"
#include "piemenu_ids.h"
#include "piemenu_data.h"
#include "piemenu_editor_area.h"

namespace cinema
{

class PieMenuEditorDialog : public GeDialog
{
public:
    PieMenuEditorDialog();
    virtual ~PieMenuEditorDialog() = default;

    virtual Bool CreateLayout() override;
    virtual Bool InitValues() override;
    virtual Bool Command(Int32 id, const BaseContainer& msg) override;

    void UpdateInspector();
    void ApplyInspectorChanges();

private:
    PieMenuEditorArea m_userArea;
};

class PieMenuEditorCommand : public CommandData
{
public:
    virtual Bool Execute(BaseDocument* doc, GeDialog* parentManager) override;
    virtual Int32 GetState(BaseDocument* doc, GeDialog* parentManager) override;
    virtual Bool RestoreLayout(void* secret) override;

    static Bool Register();

private:
    PieMenuEditorDialog m_dialog;
};

} // namespace cinema

#endif // PIEMENU_EDITOR_DIALOG_H__
