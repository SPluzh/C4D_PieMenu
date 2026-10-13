#ifndef PIEMENU_EDITOR_DIALOG_H__
#define PIEMENU_EDITOR_DIALOG_H__

#include "c4d.h"
#include "piemenu_ids.h"
#include "piemenu_data.h"
#include "piemenu_editor_area.h"
#include "c4d_customgui/customgui_listview.h"

namespace cinema
{

struct CommandItem
{
    Int32    id = 0;
    String   name;
    String   shortcut;
    IconData icon;
    Bool     hasIcon = false;
    Int32    filterIndex = 0;
};

class CommandListTreeView : public TreeViewFunctions
{
public:
    virtual void* GetFirst(void* root, void* userdata) override;
    virtual void* GetDown(void* root, void* userdata, void* obj) override;
    virtual void* GetNext(void* root, void* userdata, void* obj) override;
    virtual void* GetPred(void* root, void* userdata, void* obj) override;

    virtual Bool IsSelected(void* root, void* userdata, void* obj) override;
    virtual void Select(void* root, void* userdata, void* obj, Int32 mode) override;

    virtual Bool IsOpened(void* root, void* userdata, void* obj) override;
    virtual String GetName(void* root, void* userdata, void* obj) override;
    virtual Int GetId(void* root, void* userdata, void* obj) override;

    virtual Int32 GetDragType(void* root, void* userdata, void* obj) override;
    virtual Int32 DragStart(void* root, void* userdata, void* obj) override;
    virtual void GenerateDragData(void* root, void* userdata, void* obj, void** dragdata) override;
    virtual Int32 AcceptDragObject(void* root, void* userdata, void* obj, Int32 dragtype, void* dragobject, Bool& bAllowCopy) override;

    virtual Int32 GetColumnWidth(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area) override;
    virtual Int32 GetHeaderColumnWidth(void* root, void* userdata, Int32 col, GeUserArea* area) override;
    virtual Int32 GetLineHeight(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area) override;

    virtual void DrawCell(void* root, void* userdata, void* obj, Int32 col, DrawInfo* drawinfo, const GeData& bgColor) override;
    virtual Int32 DoubleClick(void* root, void* userdata, void* obj, Int32 col, MouseInfo* mouseinfo) override;
};

struct SubmenuItem
{
    Int32         index = 0;
    maxon::String name;
    PieContextMode context = PieContextMode::Global;
    Int32         slotCount = 8;
    maxon::String hotkeyStr;
};

class SubmenuListTreeView : public TreeViewFunctions
{
public:
    virtual void* GetFirst(void* root, void* userdata) override;
    virtual void* GetDown(void* root, void* userdata, void* obj) override;
    virtual void* GetNext(void* root, void* userdata, void* obj) override;
    virtual void* GetPred(void* root, void* userdata, void* obj) override;

    virtual Bool IsSelected(void* root, void* userdata, void* obj) override;
    virtual void Select(void* root, void* userdata, void* obj, Int32 mode) override;

    virtual Bool IsOpened(void* root, void* userdata, void* obj) override;
    virtual String GetName(void* root, void* userdata, void* obj) override;
    virtual Int GetId(void* root, void* userdata, void* obj) override;

    virtual Int32 GetDragType(void* root, void* userdata, void* obj) override;
    virtual Int32 DragStart(void* root, void* userdata, void* obj) override;
    virtual void GenerateDragData(void* root, void* userdata, void* obj, void** dragdata) override;
    virtual Int32 AcceptDragObject(void* root, void* userdata, void* obj, Int32 dragtype, void* dragobject, Bool& bAllowCopy) override;

    virtual Int32 GetColumnWidth(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area) override;
    virtual Int32 GetHeaderColumnWidth(void* root, void* userdata, Int32 col, GeUserArea* area) override;
    virtual Int32 GetLineHeight(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area) override;

    virtual void DrawCell(void* root, void* userdata, void* obj, Int32 col, DrawInfo* drawinfo, const GeData& bgColor) override;
    virtual Int32 DoubleClick(void* root, void* userdata, void* obj, Int32 col, MouseInfo* mouseinfo) override;
};

class PieMenuEditorDialog : public GeDialog
{
public:
    PieMenuEditorDialog();
    virtual ~PieMenuEditorDialog() = default;

    virtual Bool CreateLayout() override;
    virtual Bool InitValues() override;
    virtual Bool Command(Int32 id, const BaseContainer& msg) override;
    virtual Int32 Message(const BaseContainer& msg, BaseContainer& result) override;
    virtual void DestroyWindow() override;

    void UpdateInspector();
    void ApplyInspectorChanges();

    void UpdateMenuSettingsUI();
    void ApplyMenuSettingsChanges();

    void UpdateHotkeyUI();
    void ApplyHotkeyChanges();
    void SetCapturedHotkey(Bool ctrl, Bool shift, Bool alt, Int32 vkCode);
    void CancelHotkeyCapture();

    void RefreshProfileComboList();
    void SelectProfile(Int32 index);

    static Bool IsCapturingHotkey();
    static void NotifyKeyCaptured(UInt32 vkCode);
    static Bool IsAnyFieldActive();

    void AssignCommandToSelectedSlot(Int32 cmdId);
    void PopulateAllCommands();
    void FilterCommands(const String& query);

    void AssignSubmenuToSelectedSlot(const maxon::String& subMenuName);
    void PopulateAllSubmenus();
    void RefreshSubmenuList();

    const maxon::BaseArray<CommandItem*>& GetFilteredCommands() const { return m_filteredCommands; }
    CommandItem* GetSelectedCommand() const { return m_selectedCommand; }
    void SetSelectedCommand(CommandItem* item) { m_selectedCommand = item; }

    const maxon::BaseArray<SubmenuItem>& GetAllSubmenus() const { return m_allSubmenus; }
    SubmenuItem* GetSelectedSubmenu() const { return m_selectedSubmenu; }
    void SetSelectedSubmenu(SubmenuItem* item) { m_selectedSubmenu = item; }

    static PieMenuEditorDialog* s_activeDialog;
    Bool m_waitingForHotkey = false;

private:
    PieMenuEditorArea m_userArea;

    TreeViewCustomGui* m_treeGui = nullptr;
    CommandListTreeView m_treeFunctions;
    maxon::BaseArray<CommandItem> m_allCommands;
    maxon::BaseArray<CommandItem*> m_filteredCommands;
    CommandItem* m_selectedCommand = nullptr;

    TreeViewCustomGui* m_submenuTreeGui = nullptr;
    SubmenuListTreeView m_submenuTreeFunctions;
    maxon::BaseArray<SubmenuItem> m_allSubmenus;
    SubmenuItem* m_selectedSubmenu = nullptr;
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
