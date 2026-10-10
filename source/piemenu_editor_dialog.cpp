#include "piemenu_editor_dialog.h"
#include "c4d_general.h"
#include "c4d_baseplugin.h"
#include "windows_include.h"
#include <algorithm>
#include <functional>
#include <unordered_set>

namespace cinema
{

enum
{
    // Profile Management
    IDC_GROUP_MENU_MANAGE = 1000,
    IDC_PROFILE_COMBO,
    IDC_BTN_NEW_PROFILE,
    IDC_BTN_DUPLICATE_PROFILE,
    IDC_BTN_DELETE_PROFILE,

    // Active Profile Properties
    IDC_GROUP_MENU_SETTINGS,
    IDC_PROFILE_NAME_EDIT,
    IDC_PROFILE_CONTEXT_COMBO,
    IDC_SLOTCOUNT_COMBO,
    IDC_PROFILE_SHOW_TITLE,

    IDC_USERAREA_PIE,
    IDC_GROUP_INSPECTOR,
    IDC_INSPECTOR_SLOT_TITLE,
    IDC_INSPECTOR_ENABLED,
    IDC_INSPECTOR_CMD_ID,
    IDC_INSPECTOR_CMD_NAME,
    IDC_INSPECTOR_COLOR_FIELD,
    IDC_INSPECTOR_CLEAR_SLOT,
    IDC_GLOBAL_RADIUS,
    IDC_GLOBAL_DEADZONE,
    IDC_BTN_SAVE,
    IDC_BTN_EXPORT,
    IDC_BTN_IMPORT,
    IDC_BTN_RESET,
    IDC_BTN_CLOSE,

    // Left commands library panel
    IDC_GROUP_CMD_PANEL,
    IDC_CMD_SEARCH,
    IDC_CMD_COUNT,
    IDC_CMD_BTN_ASSIGN,
    IDC_CMD_TREEVIEW,

    // Menu Hotkey settings
    IDC_GROUP_MENU_HOTKEY,
    IDC_HOTKEY_CTRL,
    IDC_HOTKEY_SHIFT,
    IDC_HOTKEY_ALT,
    IDC_HOTKEY_KEY_COMBO,
    IDC_HOTKEY_ASSIGN_BTN,
    IDC_HOTKEY_CLEAR_BTN,
    IDC_HOTKEY_PREVIEW_TEXT
};

PieMenuEditorDialog* PieMenuEditorDialog::s_activeDialog = nullptr;

struct AvailableKey
{
    Int32 vkCode;
    const Char* name;
};

static const AvailableKey s_availableKeys[] = {
    { 0, "None (Modifiers only)" },
    { 'A', "A" }, { 'B', "B" }, { 'C', "C" }, { 'D', "D" }, { 'E', "E" },
    { 'F', "F" }, { 'G', "G" }, { 'H', "H" }, { 'I', "I" }, { 'J', "J" },
    { 'K', "K" }, { 'L', "L" }, { 'M', "M" }, { 'N', "N" }, { 'O', "O" },
    { 'P', "P" }, { 'Q', "Q" }, { 'R', "R" }, { 'S', "S" }, { 'T', "T" },
    { 'U', "U" }, { 'V', "V" }, { 'W', "W" }, { 'X', "X" }, { 'Y', "Y" }, { 'Z', "Z" },
    { '0', "0" }, { '1', "1" }, { '2', "2" }, { '3', "3" }, { '4', "4" },
    { '5', "5" }, { '6', "6" }, { '7', "7" }, { '8', "8" }, { '9', "9" },
#if defined(MAXON_TARGET_WINDOWS)
    { VK_TAB, "Tab" },
    { VK_SPACE, "Space" },
    { VK_OEM_3, "~ (Tilde)" },
    { VK_OEM_4, "[" },
    { VK_OEM_6, "]" },
    { VK_OEM_COMMA, "," },
    { VK_OEM_PERIOD, "." },
    { VK_OEM_1, ";" },
    { VK_OEM_7, "'" },
    { VK_OEM_2, "/" },
    { VK_OEM_MINUS, "-" },
    { VK_OEM_PLUS, "=" },
    { VK_F1, "F1" }, { VK_F2, "F2" }, { VK_F3, "F3" }, { VK_F4, "F4" },
    { VK_F5, "F5" }, { VK_F6, "F6" }, { VK_F7, "F7" }, { VK_F8, "F8" },
    { VK_F9, "F9" }, { VK_F10, "F10" }, { VK_F11, "F11" }, { VK_F12, "F12" }
#endif
};
static const Int32 s_availableKeyCount = (Int32)(sizeof(s_availableKeys) / sizeof(s_availableKeys[0]));

static Int32 VkCodeToKeyComboIndex(Int32 vkCode)
{
    Int32 target = vkCode;
    if (target >= 'a' && target <= 'z') target = toupper(target);
    for (Int32 i = 0; i < s_availableKeyCount; ++i)
    {
        if (s_availableKeys[i].vkCode == target)
            return i;
    }
    return 0; // None
}

static Int32 KeyComboIndexToVkCode(Int32 idx)
{
    if (idx >= 0 && idx < s_availableKeyCount)
        return s_availableKeys[idx].vkCode;
    return 0;
}

static const Char* s_slotNames8[9] = {
    "North (Up)",
    "North-East (Up-Right)",
    "East (Right)",
    "South-East (Down-Right)",
    "South (Down)",
    "South-West (Down-Left)",
    "West (Left)",
    "North-West (Up-Left)",
    "Center (Hub)"
};

static const Char* s_slotNames4[9] = {
    "North (Up)",
    "East (Right)",
    "South (Down)",
    "West (Left)",
    "-", "-", "-", "-",
    "Center (Hub)"
};

// ============================================================================
// COMMAND LIST TREE VIEW FUNCTIONS IMPLEMENTATION
// ============================================================================

void* CommandListTreeView::GetFirst(void* root, void* userdata)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    if (!dlg)
        return nullptr;
    const maxon::BaseArray<CommandItem*>& list = dlg->GetFilteredCommands();
    return (list.GetCount() > 0) ? list[0] : nullptr;
}

void* CommandListTreeView::GetDown(void* root, void* userdata, void* obj)
{
    return nullptr;
}

void* CommandListTreeView::GetNext(void* root, void* userdata, void* obj)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    CommandItem* item = static_cast<CommandItem*>(obj);
    if (!dlg || !item)
        return nullptr;

    const maxon::BaseArray<CommandItem*>& list = dlg->GetFilteredCommands();
    Int32 nextIdx = item->filterIndex + 1;
    if (nextIdx >= 0 && nextIdx < (Int32)list.GetCount())
        return list[nextIdx];

    return nullptr;
}

void* CommandListTreeView::GetPred(void* root, void* userdata, void* obj)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    CommandItem* item = static_cast<CommandItem*>(obj);
    if (!dlg || !item)
        return nullptr;

    const maxon::BaseArray<CommandItem*>& list = dlg->GetFilteredCommands();
    Int32 predIdx = item->filterIndex - 1;
    if (predIdx >= 0 && predIdx < (Int32)list.GetCount())
        return list[predIdx];

    return nullptr;
}

Bool CommandListTreeView::IsSelected(void* root, void* userdata, void* obj)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    return dlg ? (dlg->GetSelectedCommand() == obj) : false;
}

void CommandListTreeView::Select(void* root, void* userdata, void* obj, Int32 mode)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    if (!dlg)
        return;

    if (mode == SELECTION_NEW || mode == SELECTION_ADD)
    {
        dlg->SetSelectedCommand(static_cast<CommandItem*>(obj));
    }
    else if (mode == SELECTION_SUB)
    {
        if (dlg->GetSelectedCommand() == obj)
            dlg->SetSelectedCommand(nullptr);
    }
}

Bool CommandListTreeView::IsOpened(void* root, void* userdata, void* obj)
{
    return false;
}

String CommandListTreeView::GetName(void* root, void* userdata, void* obj)
{
    CommandItem* item = static_cast<CommandItem*>(obj);
    return item ? item->name : String();
}

Int CommandListTreeView::GetId(void* root, void* userdata, void* obj)
{
    CommandItem* item = static_cast<CommandItem*>(obj);
    return item ? (Int)item->id : 0;
}

Int32 CommandListTreeView::GetDragType(void* root, void* userdata, void* obj)
{
    return DRAGTYPE_COMMAND;
}

Int32 CommandListTreeView::DragStart(void* root, void* userdata, void* obj)
{
    return TREEVIEW_DRAGSTART_ALLOW | TREEVIEW_DRAGSTART_SELECT;
}

void CommandListTreeView::GenerateDragData(void* root, void* userdata, void* obj, void** dragdata)
{
    CommandItem* item = static_cast<CommandItem*>(obj);
    if (item && dragdata)
    {
        *dragdata = reinterpret_cast<void*>(static_cast<uintptr_t>(item->id));
    }
}

Int32 CommandListTreeView::AcceptDragObject(void* root, void* userdata, void* obj, Int32 dragtype, void* dragobject, Bool& bAllowCopy)
{
    return 0;
}

Int32 CommandListTreeView::GetColumnWidth(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area)
{
    if (col == 'icon')
        return 24;
    if (col == 'cmdi')
        return 65;
    return LV_COLUMN_WIDTH_FIT;
}

Int32 CommandListTreeView::GetHeaderColumnWidth(void* root, void* userdata, Int32 col, GeUserArea* area)
{
    if (col == 'icon')
        return 24;
    if (col == 'cmdi')
        return 65;
    return LV_COLUMN_WIDTH_FIT;
}

Int32 CommandListTreeView::GetLineHeight(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area)
{
    return 22;
}

void CommandListTreeView::DrawCell(void* root, void* userdata, void* obj, Int32 col, DrawInfo* drawinfo, const GeData& bgColor)
{
    CommandItem* item = static_cast<CommandItem*>(obj);
    if (!item || !drawinfo || !drawinfo->frame)
        return;

    if (col == 'icon')
    {
        if (item->hasIcon && item->icon.bmp)
        {
            const Int32 sz = 16;
            Int32 ix = drawinfo->xpos + (drawinfo->width - sz) / 2;
            Int32 iy = drawinfo->ypos + (drawinfo->height - sz) / 2;
            Int32 drawflags = BMP_ALLOWALPHA | BMP_NORMALSCALED;
            if (item->icon.flags & ICONDATAFLAGS::APPLYCOLORPROFILE)
                drawflags |= BMP_APPLY_COLORPROFILE;
            drawinfo->frame->DrawBitmap(item->icon.bmp, ix, iy, sz, sz,
                item->icon.x, item->icon.y, item->icon.w, item->icon.h,
                drawflags);
        }
    }
    else if (col == 'cmdi')
    {
        String idStr = String::IntToString(item->id);
        drawinfo->frame->DrawSetTextCol(Vector(0.50, 0.53, 0.58), COLOR_TRANS);
        Int32 th = drawinfo->frame->DrawGetFontHeight();
        Int32 ty = drawinfo->ypos + (drawinfo->height - th) / 2;
        drawinfo->frame->DrawText(idStr, drawinfo->xpos + 4, ty);
    }
}

Int32 CommandListTreeView::DoubleClick(void* root, void* userdata, void* obj, Int32 col, MouseInfo* mouseinfo)
{
    CommandItem* item = static_cast<CommandItem*>(obj);
    if (item && userdata)
    {
        PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
        dlg->AssignCommandToSelectedSlot(item->id);
        return true;
    }
    return false;
}

// ============================================================================
// PIE MENU EDITOR DIALOG IMPLEMENTATION
// ============================================================================

PieMenuEditorDialog::PieMenuEditorDialog()
{
    s_activeDialog = this;
}

Bool PieMenuEditorDialog::CreateLayout()
{
    s_activeDialog = this;
    SetTitle("Pie Menu Configuration"_s);

    // 1. Header / instructions
    GroupBegin(0, BFH_SCALEFIT, 1, 0, ""_s, 0);
    GroupBorderSpace(12, 10, 12, 4);
    AddStaticText(0, BFH_LEFT, 0, 0, "Cinema 4D Radial Pie Menu - Visual Configuration"_s, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Drag commands from the left list onto any slot, or double-click to assign."_s, 0);
    GroupEnd();

    // 2. Main 2-column layout (Left: Commands Library, Right: Pie Menu & Inspector)
    GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, ""_s, 0);
    GroupBorderSpace(10, 4, 10, 4);
    GroupSpace(10, 0);

    // =============================================================
    // LEFT COLUMN: Commands Library & Search
    // =============================================================
    GroupBegin(IDC_GROUP_CMD_PANEL, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "Commands"_s, BORDER_GROUP_IN);
    GroupBorderSpace(8, 8, 8, 8);
    GroupSpace(0, 6);

    // Search bar row
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Search:"_s, 0);
    AddEditText(IDC_CMD_SEARCH, BFH_SCALEFIT, 160, 0, EDITTEXT_ENABLECLEARBUTTON);
    GroupEnd();

    // Count and Quick Assign button
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    AddStaticText(IDC_CMD_COUNT, BFH_LEFT, 0, 0, "0 commands"_s, 0);
    AddButton(IDC_CMD_BTN_ASSIGN, BFH_RIGHT, 0, 0, "Assign to Slot ->"_s);
    GroupEnd();

    // TreeView of commands
    BaseContainer treedata;
    treedata.SetBool(TREEVIEW_BORDER, true);
    treedata.SetBool(TREEVIEW_HAS_HEADER, true);
    treedata.SetBool(TREEVIEW_HIDE_LINES, true);
    treedata.SetBool(TREEVIEW_FIXED_LAYOUT, true);
    treedata.SetBool(TREEVIEW_ALTERNATE_BG, true);
    treedata.SetBool(TREEVIEW_NO_MULTISELECT, true);
    treedata.SetBool(TREEVIEW_CURSORKEYS, true);
    treedata.SetBool(TREEVIEW_RESIZE_HEADER, true);
    treedata.SetBool(TREEVIEW_DRAG_START_REQUIRES_MOVEMENT, true);

    m_treeGui = (TreeViewCustomGui*)AddCustomGui(IDC_CMD_TREEVIEW, CUSTOMGUI_TREEVIEW, String(), BFH_SCALEFIT | BFV_SCALEFIT, 300, 400, treedata);

    // Bottom hint
    AddStaticText(0, BFH_SCALEFIT, 0, 0, "Drag item to slot or double-click to assign"_s, 0);

    GroupEnd(); // End IDC_GROUP_CMD_PANEL

    // =============================================================
    // RIGHT COLUMN: Pie Preview, Profile & Properties
    // =============================================================
    GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, ""_s, 0);
    GroupSpace(0, 6);

    // 1. Menu selection and management group
    GroupBegin(IDC_GROUP_MENU_MANAGE, BFH_SCALEFIT, 5, 0, "Menu Management"_s, BORDER_GROUP_IN);
    GroupBorderSpace(8, 6, 8, 6);
    GroupSpace(6, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Active Menu:"_s, 0);
    AddComboBox(IDC_PROFILE_COMBO, BFH_SCALEFIT, 200, 0);
    AddButton(IDC_BTN_NEW_PROFILE, BFH_LEFT, 75, 0, "+ New Menu"_s);
    AddButton(IDC_BTN_DUPLICATE_PROFILE, BFH_LEFT, 75, 0, "Duplicate"_s);
    AddButton(IDC_BTN_DELETE_PROFILE, BFH_LEFT, 75, 0, "- Delete"_s);
    GroupEnd();

    // 2. Active Menu Properties group (Name, Context, Slots, Title)
    GroupBegin(IDC_GROUP_MENU_SETTINGS, BFH_SCALEFIT, 7, 0, "Menu Properties"_s, BORDER_GROUP_IN);
    GroupBorderSpace(8, 6, 8, 6);
    GroupSpace(6, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Name:"_s, 0);
    AddEditText(IDC_PROFILE_NAME_EDIT, BFH_SCALEFIT, 120, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Context:"_s, 0);
    AddComboBox(IDC_PROFILE_CONTEXT_COMBO, BFH_LEFT, 140, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Slots:"_s, 0);
    AddComboBox(IDC_SLOTCOUNT_COMBO, BFH_LEFT, 80, 0);
    AddCheckbox(IDC_PROFILE_SHOW_TITLE, BFH_LEFT, 0, 0, "Show Menu Title"_s);
    GroupEnd();

    // 3. Menu Hotkey settings group for the active profile
    GroupBegin(IDC_GROUP_MENU_HOTKEY, BFH_SCALEFIT, 8, 0, "Menu Hotkey (Trigger Shortcut)"_s, BORDER_GROUP_IN);
    GroupBorderSpace(8, 6, 8, 6);
    GroupSpace(6, 0);

    AddCheckbox(IDC_HOTKEY_CTRL, BFH_LEFT, 0, 0, "Ctrl"_s);
    AddCheckbox(IDC_HOTKEY_SHIFT, BFH_LEFT, 0, 0, "Shift"_s);
    AddCheckbox(IDC_HOTKEY_ALT, BFH_LEFT, 0, 0, "Alt"_s);

    AddStaticText(0, BFH_LEFT, 0, 0, "Key:"_s, 0);
    AddComboBox(IDC_HOTKEY_KEY_COMBO, BFH_LEFT, 130, 0);

    AddButton(IDC_HOTKEY_ASSIGN_BTN, BFH_LEFT, 85, 0, "Press Key..."_s);
    AddButton(IDC_HOTKEY_CLEAR_BTN, BFH_LEFT, 50, 0, "Clear"_s);

    AddStaticText(IDC_HOTKEY_PREVIEW_TEXT, BFH_SCALEFIT, 0, 0, ""_s, 0);
    GroupEnd();

    // UserArea preview
    GroupBegin(0, BFH_CENTER, 1, 0, ""_s, 0);
    GroupBorderSpace(4, 0, 4, 4);
    AddUserArea(IDC_USERAREA_PIE, BFH_CENTER | BFV_CENTER, 360, 360);
    AttachUserArea(m_userArea, IDC_USERAREA_PIE);
    GroupEnd();

    // Selected Slot Inspector group
    GroupBegin(IDC_GROUP_INSPECTOR, BFH_SCALEFIT, 2, 0, "Selected Slot Properties"_s, BORDER_GROUP_IN);
    GroupBorderSpace(10, 6, 10, 6);

    AddStaticText(0, BFH_LEFT, 0, 0, "Slot Position:"_s, 0);
    AddStaticText(IDC_INSPECTOR_SLOT_TITLE, BFH_SCALEFIT, 0, 0, ""_s, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Enabled:"_s, 0);
    AddCheckbox(IDC_INSPECTOR_ENABLED, BFH_LEFT, 0, 0, "Slot Active in Menu"_s);

    AddStaticText(0, BFH_LEFT, 0, 0, "Command ID:"_s, 0);
    AddEditNumber(IDC_INSPECTOR_CMD_ID, BFH_SCALEFIT, 0, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Custom Label:"_s, 0);
    AddEditText(IDC_INSPECTOR_CMD_NAME, BFH_SCALEFIT, 0, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Accent Color:"_s, 0);
    AddColorField(IDC_INSPECTOR_COLOR_FIELD, BFH_LEFT, 60, 18);

    AddStaticText(0, BFH_LEFT, 0, 0, ""_s, 0);
    AddButton(IDC_INSPECTOR_CLEAR_SLOT, BFH_LEFT, 110, 0, "Clear Slot"_s);

    GroupEnd();

    // Global Settings group
    GroupBegin(0, BFH_SCALEFIT, 4, 0, "Global Behavior"_s, BORDER_GROUP_IN);
    GroupBorderSpace(10, 6, 10, 6);

    AddStaticText(0, BFH_LEFT, 0, 0, "Menu Radius (px):"_s, 0);
    AddEditNumber(IDC_GLOBAL_RADIUS, BFH_SCALEFIT, 60, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Deadzone (px):"_s, 0);
    AddEditNumber(IDC_GLOBAL_DEADZONE, BFH_SCALEFIT, 60, 0);

    GroupEnd();

    GroupEnd(); // End Right Column

    GroupEnd(); // End Main 2-column layout

    // 3. Action buttons at bottom
    GroupBegin(0, BFH_SCALEFIT, 5, 0, ""_s, 0);
    GroupBorderSpace(12, 6, 12, 10);
    AddButton(IDC_BTN_SAVE, BFH_SCALEFIT, 0, 0, "Save Configuration"_s);
    AddButton(IDC_BTN_EXPORT, BFH_SCALEFIT, 0, 0, "Export Settings..."_s);
    AddButton(IDC_BTN_IMPORT, BFH_SCALEFIT, 0, 0, "Import Settings..."_s);
    AddButton(IDC_BTN_RESET, BFH_SCALEFIT, 0, 0, "Reset to Defaults"_s);
    AddButton(IDC_BTN_CLOSE, BFH_SCALEFIT, 0, 0, "Close"_s);
    GroupEnd();

    return true;
}

void PieMenuEditorDialog::PopulateAllCommands()
{
    if (m_allCommands.GetCount() > 0)
        return;

    std::unordered_set<Int32> seen;

    // 1. Collect CommandData plugins
    AutoAlloc<AtomArray> cmdArray;
    if (cmdArray && FilterPluginList(*cmdArray, PLUGINTYPE::COMMAND, true))
    {
        for (Int32 i = 0; i < cmdArray->GetCount(); ++i)
        {
            BasePlugin* bp = static_cast<BasePlugin*>(cmdArray->GetIndex(i));
            if (!bp)
                continue;

            Int32 id = bp->GetID();
            if (id <= 0 || (bp->GetInfo() & PLUGINFLAG_HIDE))
                continue;
            if (seen.find(id) != seen.end())
                continue;

            String name = bp->GetName();
            if (!name.IsPopulated())
                name = GetCommandName(id);
            if (!name.IsPopulated())
                continue;

            seen.insert(id);
            CommandItem item;
            item.id = id;
            item.name = name;
            item.hasIcon = GetIcon(id, &item.icon) && (item.icon.bmp != nullptr);
            m_allCommands.Append(item) iferr_ignore("Append");
        }
    }

    // 2. Collect ToolData plugins
    AutoAlloc<AtomArray> toolArray;
    if (toolArray && FilterPluginList(*toolArray, PLUGINTYPE::TOOL, true))
    {
        for (Int32 i = 0; i < toolArray->GetCount(); ++i)
        {
            BasePlugin* bp = static_cast<BasePlugin*>(toolArray->GetIndex(i));
            if (!bp)
                continue;

            Int32 id = bp->GetID();
            if (id <= 0 || (bp->GetInfo() & PLUGINFLAG_HIDE))
                continue;
            if (seen.find(id) != seen.end())
                continue;

            String name = bp->GetName();
            if (!name.IsPopulated())
                name = GetCommandName(id);
            if (!name.IsPopulated())
                continue;

            seen.insert(id);
            CommandItem item;
            item.id = id;
            item.name = name;
            item.hasIcon = GetIcon(id, &item.icon) && (item.icon.bmp != nullptr);
            m_allCommands.Append(item) iferr_ignore("Append");
        }
    }

    // 3. Fallback: traverse full plugin hierarchy
    std::function<void(BasePlugin*)> recurse = [&](BasePlugin* bp)
    {
        for (; bp; bp = bp->GetNext())
        {
            PLUGINTYPE type = bp->GetPluginType();
            if (type == PLUGINTYPE::COMMAND || type == PLUGINTYPE::TOOL)
            {
                Int32 id = bp->GetID();
                if (id > 0 && !(bp->GetInfo() & PLUGINFLAG_HIDE) && seen.find(id) == seen.end())
                {
                    String name = bp->GetName();
                    if (!name.IsPopulated())
                        name = GetCommandName(id);
                    if (name.IsPopulated())
                    {
                        seen.insert(id);
                        CommandItem item;
                        item.id = id;
                        item.name = name;
                        item.hasIcon = GetIcon(id, &item.icon) && (item.icon.bmp != nullptr);
                        m_allCommands.Append(item) iferr_ignore("Append");
                    }
                }
            }
            if (bp->GetDown())
                recurse(bp->GetDown());
        }
    };
    recurse(GetFirstPlugin());

    // Sort alphabetically by name
    std::sort(m_allCommands.Begin(), m_allCommands.End(), [](const CommandItem& a, const CommandItem& b)
    {
        return a.name.Compare(b.name, maxon::STRINGCOMPARISON::CASEINSENSITIVE_NUMBERS) == maxon::COMPARERESULT::LESS;
    });
}

void PieMenuEditorDialog::FilterCommands(const String& query)
{
    m_filteredCommands.Reset();
    String q = query;
    q = q.ToLower();

    for (CommandItem& item : m_allCommands)
    {
        Bool match = false;
        if (q.IsEmpty())
        {
            match = true;
        }
        else
        {
            if (item.name.ToLower().Find(q, nullptr))
            {
                match = true;
            }
            else
            {
                String idStr = String::IntToString(item.id);
                if (idStr.Find(q, nullptr))
                    match = true;
            }
        }

        if (match)
        {
            item.filterIndex = (Int32)m_filteredCommands.GetCount();
            m_filteredCommands.Append(&item) iferr_ignore("Append");
        }
    }

    // Update count label
    String countStr;
    if (q.IsEmpty())
    {
        countStr = FormatString("@ commands"_s, (Int32)m_filteredCommands.GetCount());
    }
    else
    {
        countStr = FormatString("@ / @ commands"_s, (Int32)m_filteredCommands.GetCount(), (Int32)m_allCommands.GetCount());
    }
    SetString(IDC_CMD_COUNT, countStr);

    if (m_treeGui)
    {
        m_treeGui->Refresh();
    }
}

void PieMenuEditorDialog::AssignCommandToSelectedSlot(Int32 cmdId)
{
    if (cmdId <= 0)
        return;

    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    Int32 sel = m_userArea.GetSelectedSlot();
    if (sel < 0 || sel > 8)
        sel = 0;

    prof->slots[sel].commandId = cmdId;
    prof->slots[sel].customLabel = GetCommandName(cmdId);
    prof->slots[sel].enabled = true;

    UpdateInspector();
    m_userArea.Redraw();
}

Bool PieMenuEditorDialog::InitValues()
{
    PieProfileManager& mgr = PieProfileManager::Get();

    // Populate key selection combo
    FreeChildren(IDC_HOTKEY_KEY_COMBO);
    for (Int32 i = 0; i < s_availableKeyCount; ++i)
    {
        AddChild(IDC_HOTKEY_KEY_COMBO, i, String(s_availableKeys[i].name));
    }

    // Populate context combo
    FreeChildren(IDC_PROFILE_CONTEXT_COMBO);
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Global, GetContextName(PieContextMode::Global));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Polygons, GetContextName(PieContextMode::Polygons));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Edges, GetContextName(PieContextMode::Edges));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Points, GetContextName(PieContextMode::Points));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Objects, GetContextName(PieContextMode::Objects));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::UV, GetContextName(PieContextMode::UV));

    // Populate slot counts
    FreeChildren(IDC_SLOTCOUNT_COMBO);
    AddChild(IDC_SLOTCOUNT_COMBO, 0, "4 Slots"_s);
    AddChild(IDC_SLOTCOUNT_COMBO, 1, "8 Slots"_s);

    // Global settings
    SetFloat(IDC_GLOBAL_RADIUS, mgr.m_radius);
    SetFloat(IDC_GLOBAL_DEADZONE, mgr.m_deadzone);

    // Populate and initialize commands library
    PopulateAllCommands();
    FilterCommands(""_s);

    if (m_treeGui)
    {
        BaseContainer layout;
        layout.SetInt32('icon', LV_USER);
        layout.SetInt32('tree', LV_TREE);
        layout.SetInt32('cmdi', LV_USER);
        m_treeGui->SetLayout(3, layout);
        m_treeGui->SetHeaderText('icon', ""_s);
        m_treeGui->SetHeaderText('tree', "Command"_s);
        m_treeGui->SetHeaderText('cmdi', "ID"_s);

        m_treeGui->SetRoot(this, &m_treeFunctions, this);
        m_treeGui->Refresh();
    }

    RefreshProfileComboList();
    SelectProfile(m_userArea.GetActiveProfileIndex());
    return true;
}

void PieMenuEditorDialog::UpdateMenuSettingsUI()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    Int32 curIdx = m_userArea.GetActiveProfileIndex();
    PieProfile* prof = mgr.GetProfile(curIdx);
    if (!prof)
        return;

    SetString(IDC_PROFILE_NAME_EDIT, prof->name);
    SetInt32(IDC_PROFILE_CONTEXT_COMBO, (Int32)prof->context);
    SetInt32(IDC_SLOTCOUNT_COMBO, (prof->slotCount == 4) ? 0 : 1);
    SetBool(IDC_PROFILE_SHOW_TITLE, prof->showTitle);
}

void PieMenuEditorDialog::ApplyMenuSettingsChanges()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    Int32 curIdx = m_userArea.GetActiveProfileIndex();
    PieProfile* prof = mgr.GetProfile(curIdx);
    if (!prof)
        return;

    String name;
    if (GetString(IDC_PROFILE_NAME_EDIT, name) && name.IsPopulated())
        prof->name = name;

    Int32 ctx = 0;
    if (GetInt32(IDC_PROFILE_CONTEXT_COMBO, ctx))
        prof->context = (PieContextMode)ctx;

    Int32 sc = 0;
    if (GetInt32(IDC_SLOTCOUNT_COMBO, sc))
        prof->slotCount = (sc == 0) ? 4 : 8;

    Bool st = true;
    if (GetBool(IDC_PROFILE_SHOW_TITLE, st))
        prof->showTitle = st;

    RefreshProfileComboList();
    m_userArea.Redraw();
    UpdateInspector();
}

void PieMenuEditorDialog::SelectProfile(Int32 index)
{
    PieProfileManager& mgr = PieProfileManager::Get();
    if (index < 0 || index >= mgr.GetProfileCount())
        index = 0;

    m_userArea.SetActiveProfileIndex(index);
    SetInt32(IDC_PROFILE_COMBO, index);
    UpdateMenuSettingsUI();
    UpdateHotkeyUI();
    UpdateInspector();
    m_userArea.Redraw();
}

void PieMenuEditorDialog::RefreshProfileComboList()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    Int32 curSel = m_userArea.GetActiveProfileIndex();
    FreeChildren(IDC_PROFILE_COMBO);
    for (Int32 i = 0; i < mgr.GetProfileCount(); ++i)
    {
        PieProfile* p = mgr.GetProfile(i);
        if (p)
        {
            maxon::String ctxStr = GetContextShortName(p->context);
            maxon::String hkStr = p->hotkey.GetDisplayString();
            maxon::String title = FormatString("@ [@, @]"_s, p->name, ctxStr, hkStr);
            AddChild(IDC_PROFILE_COMBO, i, title);
        }
    }
    if (curSel >= 0 && curSel < mgr.GetProfileCount())
        SetInt32(IDC_PROFILE_COMBO, curSel);
}

void PieMenuEditorDialog::UpdateHotkeyUI()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    SetBool(IDC_HOTKEY_CTRL, prof->hotkey.ctrl);
    SetBool(IDC_HOTKEY_SHIFT, prof->hotkey.shift);
    SetBool(IDC_HOTKEY_ALT, prof->hotkey.alt);

    Int32 comboIdx = VkCodeToKeyComboIndex(prof->hotkey.vkCode);
    SetInt32(IDC_HOTKEY_KEY_COMBO, comboIdx);

    String displayStr = prof->hotkey.GetDisplayString();
    String previewText = FormatString("Assigned: [@]"_s, displayStr);
    SetString(IDC_HOTKEY_PREVIEW_TEXT, previewText);
}

void PieMenuEditorDialog::ApplyHotkeyChanges()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    Bool c = false, s = false, a = false;
    GetBool(IDC_HOTKEY_CTRL, c);
    GetBool(IDC_HOTKEY_SHIFT, s);
    GetBool(IDC_HOTKEY_ALT, a);

    Int32 comboIdx = 0;
    GetInt32(IDC_HOTKEY_KEY_COMBO, comboIdx);
    Int32 vk = KeyComboIndexToVkCode(comboIdx);

    prof->hotkey.ctrl = c;
    prof->hotkey.shift = s;
    prof->hotkey.alt = a;
    prof->hotkey.vkCode = vk;
    prof->hotkey.enabled = (c || s || a || vk != 0);

    String displayStr = prof->hotkey.GetDisplayString();
    SetString(IDC_HOTKEY_PREVIEW_TEXT, FormatString("Assigned: [@]"_s, displayStr));

    RefreshProfileComboList();
}

Bool PieMenuEditorDialog::IsCapturingHotkey()
{
    return (s_activeDialog != nullptr && s_activeDialog->IsOpen() && s_activeDialog->m_waitingForHotkey);
}

Bool PieMenuEditorDialog::IsAnyFieldActive()
{
    if (!s_activeDialog || !s_activeDialog->IsOpen())
        return false;

    if (s_activeDialog->IsActive(IDC_PROFILE_NAME_EDIT) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_SLOT_TITLE) ||
        s_activeDialog->IsActive(IDC_CMD_SEARCH))
    {
        return true;
    }

    return false;
}

void PieMenuEditorDialog::NotifyKeyCaptured(UInt32 vkCode)
{
    if (!s_activeDialog)
        return;

#if defined(MAXON_TARGET_WINDOWS)
    bool c = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool s = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool a = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    s_activeDialog->SetCapturedHotkey(c, s, a, (Int32)vkCode);
#endif
}

void PieMenuEditorDialog::SetCapturedHotkey(Bool ctrl, Bool shift, Bool alt, Int32 vkCode)
{
    m_waitingForHotkey = false;

    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    prof->hotkey.ctrl = ctrl;
    prof->hotkey.shift = shift;
    prof->hotkey.alt = alt;
    prof->hotkey.vkCode = vkCode;
    prof->hotkey.enabled = (ctrl || shift || alt || vkCode != 0);

    UpdateHotkeyUI();
    RefreshProfileComboList();
}

void PieMenuEditorDialog::CancelHotkeyCapture()
{
    m_waitingForHotkey = false;
    UpdateHotkeyUI();
}

void PieMenuEditorDialog::UpdateInspector()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    SetInt32(IDC_SLOTCOUNT_COMBO, (prof->slotCount == 4) ? 0 : 1);

    Int32 sel = m_userArea.GetSelectedSlot();
    if (sel < 0 || sel > 8)
        sel = 0;

    const Char* slotName = (prof->slotCount == 4) ? s_slotNames4[sel] : s_slotNames8[sel];
    SetString(IDC_INSPECTOR_SLOT_TITLE, String(slotName));

    const PieSlot& slot = prof->slots[sel];
    SetBool(IDC_INSPECTOR_ENABLED, slot.enabled);
    SetInt32(IDC_INSPECTOR_CMD_ID, slot.commandId > 0 ? slot.commandId : 0);
    SetString(IDC_INSPECTOR_CMD_NAME, slot.customLabel);
    SetColorField(IDC_INSPECTOR_COLOR_FIELD, slot.accentColor, 1.0, 1.0, 0);
}

void PieMenuEditorDialog::ApplyInspectorChanges()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    Int32 sel = m_userArea.GetSelectedSlot();
    if (sel < 0 || sel > 8)
        return;

    PieSlot& slot = prof->slots[sel];
    Bool en = true;
    if (GetBool(IDC_INSPECTOR_ENABLED, en))
        slot.enabled = en;

    Int32 newCmdId = 0;
    if (GetInt32(IDC_INSPECTOR_CMD_ID, newCmdId))
    {
        slot.commandId = newCmdId > 0 ? newCmdId : NOTOK;
        if (slot.commandId > 0 && !slot.customLabel.IsPopulated())
        {
            slot.customLabel = GetCommandName(slot.commandId);
            SetString(IDC_INSPECTOR_CMD_NAME, slot.customLabel);
        }
    }

    String customName;
    if (GetString(IDC_INSPECTOR_CMD_NAME, customName))
        slot.customLabel = customName;

    m_userArea.Redraw();
}

Bool PieMenuEditorDialog::Command(Int32 id, const BaseContainer& msg)
{
    PieProfileManager& mgr = PieProfileManager::Get();

    switch (id)
    {
        case IDC_CMD_SEARCH:
        {
            String q;
            if (GetString(IDC_CMD_SEARCH, q))
                FilterCommands(q);
            break;
        }

        case IDC_CMD_BTN_ASSIGN:
        {
            if (m_selectedCommand)
                AssignCommandToSelectedSlot(m_selectedCommand->id);
            break;
        }

        case IDC_PROFILE_COMBO:
        {
            Int32 p = 0;
            if (GetInt32(IDC_PROFILE_COMBO, p))
            {
                SelectProfile(p);
            }
            break;
        }

        case IDC_BTN_NEW_PROFILE:
        {
            Int32 newIdx = mgr.AddProfile();
            RefreshProfileComboList();
            SelectProfile(newIdx);
            break;
        }

        case IDC_BTN_DUPLICATE_PROFILE:
        {
            Int32 curIdx = m_userArea.GetActiveProfileIndex();
            Int32 newIdx = mgr.DuplicateProfile(curIdx);
            if (newIdx != NOTOK)
            {
                RefreshProfileComboList();
                SelectProfile(newIdx);
            }
            break;
        }

        case IDC_BTN_DELETE_PROFILE:
        {
            if (mgr.GetProfileCount() <= 1)
            {
                MessageDialog("Cannot delete the only remaining pie menu."_s);
                break;
            }

            Int32 curIdx = m_userArea.GetActiveProfileIndex();
            PieProfile* prof = mgr.GetProfile(curIdx);
            String name = prof ? prof->name : "this menu"_s;
            if (QuestionDialog(FormatString("Are you sure you want to delete '@'?"_s, name)))
            {
                mgr.DeleteProfile(curIdx);
                Int32 nextIdx = curIdx;
                if (nextIdx >= mgr.GetProfileCount())
                    nextIdx = mgr.GetProfileCount() - 1;
                RefreshProfileComboList();
                SelectProfile(nextIdx);
            }
            break;
        }

        case IDC_PROFILE_NAME_EDIT:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                String newName;
                if (GetString(IDC_PROFILE_NAME_EDIT, newName) && newName.IsPopulated())
                {
                    prof->name = newName;
                    RefreshProfileComboList();
                }
            }
            break;
        }

        case IDC_PROFILE_CONTEXT_COMBO:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 ctx = 0;
                if (GetInt32(IDC_PROFILE_CONTEXT_COMBO, ctx))
                {
                    prof->context = (PieContextMode)ctx;
                    RefreshProfileComboList();
                }
            }
            break;
        }

        case IDC_HOTKEY_CTRL:
        case IDC_HOTKEY_SHIFT:
        case IDC_HOTKEY_ALT:
        case IDC_HOTKEY_KEY_COMBO:
        {
            ApplyHotkeyChanges();
            break;
        }

        case IDC_HOTKEY_ASSIGN_BTN:
        {
            m_waitingForHotkey = true;
            SetString(IDC_HOTKEY_PREVIEW_TEXT, ">> Press hotkey on keyboard (Esc cancels) <<"_s);
            break;
        }

        case IDC_HOTKEY_CLEAR_BTN:
        {
            m_waitingForHotkey = false;
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                prof->hotkey.ctrl = false;
                prof->hotkey.shift = false;
                prof->hotkey.alt = false;
                prof->hotkey.vkCode = 0;
                prof->hotkey.enabled = false;
                UpdateHotkeyUI();
                RefreshProfileComboList();
            }
            break;
        }

        case IDC_SLOTCOUNT_COMBO:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sc = 0;
                if (GetInt32(IDC_SLOTCOUNT_COMBO, sc))
                {
                    prof->slotCount = (sc == 0) ? 4 : 8;
                    m_userArea.Redraw();
                    UpdateInspector();
                }
            }
            break;
        }

        case IDC_PROFILE_SHOW_TITLE:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Bool st = true;
                if (GetBool(IDC_PROFILE_SHOW_TITLE, st))
                {
                    prof->showTitle = st;
                    m_userArea.Redraw();
                }
            }
            break;
        }

        case IDC_USERAREA_PIE:
        {
            UpdateInspector();
            break;
        }

        case IDC_INSPECTOR_ENABLED:
        case IDC_INSPECTOR_CMD_ID:
        case IDC_INSPECTOR_CMD_NAME:
        {
            ApplyInspectorChanges();
            break;
        }

        case IDC_INSPECTOR_COLOR_FIELD:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && sel <= 8)
                {
                    Vector col;
                    Float br = 1.0;
                    if (GetColorField(IDC_INSPECTOR_COLOR_FIELD, col, br))
                    {
                        prof->slots[sel].accentColor = col;
                        m_userArea.Redraw();
                    }
                }
            }
            break;
        }

        case IDC_INSPECTOR_CLEAR_SLOT:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && sel <= 8)
                {
                    prof->slots[sel].commandId = NOTOK;
                    prof->slots[sel].customLabel = ""_s;
                    UpdateInspector();
                    m_userArea.Redraw();
                }
            }
            break;
        }


        case IDC_GLOBAL_RADIUS:
        {
            Float r = 85.0;
            if (GetFloat(IDC_GLOBAL_RADIUS, r))
                mgr.m_radius = r;
            break;
        }

        case IDC_GLOBAL_DEADZONE:
        {
            Float dz = 22.0;
            if (GetFloat(IDC_GLOBAL_DEADZONE, dz))
                mgr.m_deadzone = dz;
            break;
        }

        case IDC_BTN_SAVE:
        {
            ApplyInspectorChanges();
            if (mgr.SaveToFile())
                GePrint("PieMenu: Successfully saved configuration to preferences."_s);
            break;
        }

        case IDC_BTN_EXPORT:
        {
            ApplyInspectorChanges();
            ApplyMenuSettingsChanges();
            ApplyHotkeyChanges();
            Float r = 85.0;
            if (GetFloat(IDC_GLOBAL_RADIUS, r))
                mgr.m_radius = r;
            Float dz = 22.0;
            if (GetFloat(IDC_GLOBAL_DEADZONE, dz))
                mgr.m_deadzone = dz;

            Int32 profileCount = mgr.GetProfileCount();
            Int32 exportTarget = NOTOK; // NOTOK means all

            if (profileCount > 1)
            {
                Int32 curIdx = m_userArea.GetActiveProfileIndex();
                PieProfile* curProf = mgr.GetProfile(curIdx);
                String curName = curProf ? curProf->name : "active menu"_s;

                String q = FormatString("Do you want to export ALL @ menu profiles?\n\n- Click 'Yes' to export ALL menus and global settings.\n- Click 'No' to export ONLY the active menu ('@')."_s, profileCount, curName);
                Bool exportAll = QuestionDialog(q);
                if (!exportAll)
                {
                    exportTarget = curIdx;
                }
            }

            Filename fn;
            if (exportTarget != NOTOK)
            {
                PieProfile* p = mgr.GetProfile(exportTarget);
                String safeName = p ? p->name : "Menu"_s;
                fn = Filename(FormatString("PieMenu_@.json"_s, safeName));
            }
            else
            {
                fn = Filename("PieMenu_Settings.json"_s);
            }

            if (!fn.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::SAVE, "Export Pie Menu Settings"_s, "json"_s))
                break;

            if (fn.GetSuffix().IsEmpty())
                fn.SetSuffix("json"_s);

            if (mgr.ExportSettingsToFile(fn, exportTarget))
            {
                MessageDialog(FormatString("Pie menu settings successfully exported to:\n\n@"_s, fn.GetString()));
            }
            else
            {
                MessageDialog("Failed to export pie menu settings to file."_s);
            }
            break;
        }

        case IDC_BTN_IMPORT:
        {
            Filename fn;
            if (!fn.FileSelect(FILESELECTTYPE::ANYTHING, FILESELECT::LOAD, "Import Pie Menu Settings"_s, "json"_s))
                break;

            Int32 countInFile = 0;
            String firstName;
            Bool hasSettings = false;

            if (!mgr.InspectImportFile(fn, countInFile, firstName, hasSettings))
            {
                MessageDialog("The selected file is not a valid Pie Menu configuration or could not be read."_s);
                break;
            }

            Bool replace = false;
            if (countInFile > 1)
            {
                String q = FormatString("The file contains @ menu profiles.\n\nDo you want to REPLACE all your current menus?\n\n- Click 'Yes' to Replace all existing menus.\n- Click 'No' to Append/Add these menus to your list."_s, countInFile);
                replace = QuestionDialog(q);
            }
            else
            {
                String q = FormatString("The file contains menu '@'.\n\nDo you want to REPLACE all your current menus with this menu, or APPEND it to your list?\n\n- Click 'Yes' to Replace all menus.\n- Click 'No' to Append to your list."_s, firstName);
                replace = QuestionDialog(q);
            }

            Int32 importedCount = 0;
            if (mgr.ImportSettingsFromFile(fn, replace, importedCount) && importedCount > 0)
            {
                mgr.SaveToFile();

                // Refresh UI
                RefreshProfileComboList();
                Int32 targetIndex = replace ? 0 : (mgr.GetProfileCount() - importedCount);
                if (targetIndex < 0) targetIndex = 0;
                SelectProfile(targetIndex);

                SetFloat(IDC_GLOBAL_RADIUS, mgr.m_radius);
                SetFloat(IDC_GLOBAL_DEADZONE, mgr.m_deadzone);

                m_userArea.Redraw();
                UpdateInspector();
                UpdateHotkeyUI();

                MessageDialog(FormatString("Successfully imported @ menu profile(s)!"_s, importedCount));
            }
            else
            {
                MessageDialog("Failed to import pie menu configuration from the file."_s);
            }
            break;
        }

        case IDC_BTN_RESET:
        {
            mgr.InitDefaults();
            mgr.SaveToFile();
            InitValues();
            m_userArea.Redraw();
            UpdateHotkeyUI();
            GePrint("PieMenu: Reset all profiles to factory defaults."_s);
            break;
        }

        case IDC_BTN_CLOSE:
        {
            Close();
            break;
        }
    }

    return GeDialog::Command(id, msg);
}

Int32 PieMenuEditorDialog::Message(const BaseContainer& msg, BaseContainer& result)
{
    switch (msg.GetId())
    {
        case BFM_INPUT:
        {
            if (m_waitingForHotkey)
            {
                Int32 dev = msg.GetInt32(BFM_INPUT_DEVICE);
                if (dev == BFM_INPUT_KEYBOARD)
                {
                    Int32 channel = msg.GetInt32(BFM_INPUT_CHANNEL);
                    if (channel == KEY_ESC)
                    {
                        CancelHotkeyCapture();
                        return true;
                    }
                }
            }
            break;
        }

        case BFM_ACTION:
        {
            if (msg.GetInt32(BFM_ACTION_ID) == IDC_CMD_SEARCH)
            {
                String q;
                if (GetString(IDC_CMD_SEARCH, q))
                    FilterCommands(q);
            }
            else if (msg.GetInt32(BFM_ACTION_ID) == IDC_PROFILE_NAME_EDIT)
            {
                PieProfileManager& mgr = PieProfileManager::Get();
                PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                if (prof)
                {
                    String newName;
                    if (GetString(IDC_PROFILE_NAME_EDIT, newName) && newName.IsPopulated())
                    {
                        prof->name = newName;
                        RefreshProfileComboList();
                    }
                }
            }
            break;
        }
    }

    return GeDialog::Message(msg, result);
}

void PieMenuEditorDialog::DestroyWindow()
{
    // Apply any pending inspector edits before saving
    ApplyInspectorChanges();

    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (prof)
    {
        String newName;
        if (GetString(IDC_PROFILE_NAME_EDIT, newName) && newName.IsPopulated())
            prof->name = newName;
    }

    Float r = 85.0;
    if (GetFloat(IDC_GLOBAL_RADIUS, r))
        mgr.m_radius = r;

    Float dz = 22.0;
    if (GetFloat(IDC_GLOBAL_DEADZONE, dz))
        mgr.m_deadzone = dz;


    // Automatically save configuration on dialog close
    if (mgr.SaveToFile())
    {
        GePrint("PieMenu: Configuration automatically saved on dialog close."_s);
    }

    m_waitingForHotkey = false;

    GeDialog::DestroyWindow();
}

// ============================================================================
// COMMAND PLUGIN IMPLEMENTATION
// ============================================================================

Bool PieMenuEditorCommand::Register()
{
    return RegisterCommandPlugin(
        PLUGIN_ID_PIEMENU_EDITOR_COMMAND,
        "Pie Menu Configuration"_s,
        0,
        nullptr,
        "Open graphical Pie Menu profile editor with Drag & Drop"_s,
        NewObjClear(PieMenuEditorCommand)
    );
}

Int32 PieMenuEditorCommand::GetState(BaseDocument* doc, GeDialog* parentManager)
{
    return CMD_ENABLED;
}

Bool PieMenuEditorCommand::Execute(BaseDocument* doc, GeDialog* parentManager)
{
    return m_dialog.Open(DLG_TYPE::ASYNC, PLUGIN_ID_PIEMENU_EDITOR_COMMAND, -1, -1, 840, 700);
}

Bool PieMenuEditorCommand::RestoreLayout(void* secret)
{
    return m_dialog.RestoreLayout(PLUGIN_ID_PIEMENU_EDITOR_COMMAND, 0, secret);
}

} // namespace cinema
