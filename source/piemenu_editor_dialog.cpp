#include "piemenu_editor_dialog.h"
#include "c4d_general.h"
#include "c4d_baseplugin.h"
#include "windows_include.h"
#include "maxon/assets.h"
#include "maxon/base_preset_asset.h"
#include "maxon/asset_draganddrop.h"
#include "maxon/asset_command.h"
#include "maxon/datadescription_string.h"
#include "maxon/subtype_asset.h"
#include <algorithm>
#include <functional>
#include <unordered_set>
#include <unordered_map>
#include <vector>

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
    IDC_SLOTCOUNT_NUMBER,
    IDC_SLOTCOUNT_SLIDER,
    IDC_PROFILE_SHOW_TITLE,
    IDC_PROFILE_SHOW_ITEM_NAME,
    IDC_PROFILE_STAY_OPEN_ON_RELEASE,
    IDC_PROFILE_STAY_OPEN_CENTER,

    IDC_USERAREA_PIE,
    IDC_GROUP_INSPECTOR,
    IDC_INSPECTOR_SLOT_TITLE,
    IDC_INSPECTOR_ENABLED,
    IDC_INSPECTOR_SUBMENU_COMBO,
    IDC_INSPECTOR_GOTO_SUBMENU,
    IDC_GROUP_MAIN_ACTION,
    IDC_INSPECTOR_MAIN_ICON,
    IDC_INSPECTOR_CMD_ID,
    IDC_INSPECTOR_CMD_NAME,
    IDC_INSPECTOR_PRESET_COMBO,
    IDC_INSPECTOR_PRESET_CAPTURE_BTN,
    IDC_INSPECTOR_PRESET_CLEAR_BTN,
    IDC_INSPECTOR_PRESET_STATUS_TEXT,
    IDC_INSPECTOR_COLOR_FIELD,

    IDC_GROUP_SHIFT_ACTION,
    IDC_INSPECTOR_SHIFT_ICON,
    IDC_INSPECTOR_SHIFT_NAME,
    IDC_INSPECTOR_SHIFT_ID,
    IDC_INSPECTOR_SHIFT_LABEL,
    IDC_INSPECTOR_SHIFT_SET,
    IDC_INSPECTOR_SHIFT_CLEAR,
    IDC_INSPECTOR_SHIFT_PRESET_COMBO,
    IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN,
    IDC_INSPECTOR_SHIFT_PRESET_CLEAR_BTN,
    IDC_INSPECTOR_SHIFT_PRESET_STATUS_TEXT,

    IDC_GROUP_CTRL_ACTION,
    IDC_INSPECTOR_CTRL_ICON,
    IDC_INSPECTOR_CTRL_NAME,
    IDC_INSPECTOR_CTRL_ID,
    IDC_INSPECTOR_CTRL_LABEL,
    IDC_INSPECTOR_CTRL_SET,
    IDC_INSPECTOR_CTRL_CLEAR,
    IDC_INSPECTOR_CTRL_PRESET_COMBO,
    IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN,
    IDC_INSPECTOR_CTRL_PRESET_CLEAR_BTN,
    IDC_INSPECTOR_CTRL_PRESET_STATUS_TEXT,

    IDC_GROUP_ALT_ACTION,
    IDC_INSPECTOR_ALT_ICON,
    IDC_INSPECTOR_ALT_NAME,
    IDC_INSPECTOR_ALT_ID,
    IDC_INSPECTOR_ALT_LABEL,
    IDC_INSPECTOR_ALT_SET,
    IDC_INSPECTOR_ALT_CLEAR,
    IDC_INSPECTOR_ALT_PRESET_COMBO,
    IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN,
    IDC_INSPECTOR_ALT_PRESET_CLEAR_BTN,
    IDC_INSPECTOR_ALT_PRESET_STATUS_TEXT,

    IDC_INSPECTOR_CLEAR_SLOT,
    IDC_GLOBAL_RADIUS,
    IDC_GLOBAL_INNER_RADIUS,
    IDC_GLOBAL_DEADZONE,
    IDC_GLOBAL_ICON_SIZE,
    IDC_GLOBAL_OPACITY,
    IDC_GLOBAL_DISPLAY_MODE,
    IDC_BTN_SAVE,
    IDC_BTN_EXPORT,
    IDC_BTN_IMPORT,
    IDC_BTN_CLOSE,

    // Left panel and tabs
    IDC_GROUP_CMD_PANEL,
    IDC_LEFT_TAB_GROUP,
    IDC_TAB_COMMANDS,
    IDC_CMD_SEARCH,
    IDC_CMD_COUNT,
    IDC_CMD_BTN_ASSIGN,
    IDC_CMD_TREEVIEW,

    // Left panel submenu tab
    IDC_TAB_SUBMENUS,
    IDC_SUBMENU_COUNT,
    IDC_SUBMENU_BTN_ASSIGN,
    IDC_SUBMENU_BTN_NEW,
    IDC_SUBMENU_TREEVIEW,

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

static maxon::String GetSlotDisplayName(Int32 slotCount, Int32 sel)
{
    if (sel == CENTER_SLOT_INDEX)
        return "Center (Hub)"_s;

    if (slotCount < MIN_SLOT_COUNT) slotCount = MIN_SLOT_COUNT;
    if (slotCount > MAX_SLOT_COUNT) slotCount = MAX_SLOT_COUNT;

    if (sel < 0 || sel >= slotCount)
        return "Invalid Slot"_s;

    if (slotCount == 4)
    {
        switch (sel)
        {
            case 0: return "North (Up)"_s;
            case 1: return "East (Right)"_s;
            case 2: return "South (Down)"_s;
            case 3: return "West (Left)"_s;
            default: break;
        }
    }
    else if (slotCount == 8)
    {
        switch (sel)
        {
            case 0: return "North (Up)"_s;
            case 1: return "North-East (Up-Right)"_s;
            case 2: return "East (Right)"_s;
            case 3: return "South-East (Down-Right)"_s;
            case 4: return "South (Down)"_s;
            case 5: return "South-West (Down-Left)"_s;
            case 6: return "West (Left)"_s;
            case 7: return "North-West (Up-Left)"_s;
            default: break;
        }
    }

    // Dynamic 16-point compass & clock angles for arbitrary slot counts (2..24)
    Float deg = (Float)sel * (360.0 / (Float)slotCount);
    static const Char* s_compass16[16] = {
        "N (12:00)", "NNE", "NE", "ENE",
        "E (3:00)",  "ESE", "SE", "SSE",
        "S (6:00)",  "SSW", "SW", "WSW",
        "W (9:00)",  "WNW", "NW", "NNW"
    };
    Int32 compIdx = (Int32)((deg + 11.25) / 22.5) % 16;
    if (compIdx < 0) compIdx = 0;

    return FormatString("Slot @: @ (@deg)"_s, sel + 1, s_compass16[compIdx], (Int32)deg);
}

// ============================================================================
// COMMAND ICON AREA IMPLEMENTATION
// ============================================================================

Bool CommandIconArea::GetMinSize(Int32& w, Int32& h)
{
    w = 34;
    h = 34;
    return true;
}

void CommandIconArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
    Int32 w = GetWidth();
    Int32 h = GetHeight();

    // Dark recessed well background
    Vector wellBg = Vector(0.12, 0.12, 0.14);
    if (!m_enabled)
        wellBg = Vector(0.17, 0.17, 0.18);
    else if (m_dragHover)
        wellBg = Vector(0.15, 0.22, 0.32);

    DrawSetPen(wellBg);
    DrawRectangle(0, 0, w, h);

    if (m_cmdId > 0 && m_enabled)
    {
        IconData icon;
        if (GetIcon(m_cmdId, &icon) && icon.bmp)
        {
            Int32 drawflags = BMP_ALLOWALPHA | BMP_NORMALSCALED;
            if (icon.flags & ICONDATAFLAGS::APPLYCOLORPROFILE)
                drawflags |= BMP_APPLY_COLORPROFILE;

            Int32 minDim = (w < h ? w : h);
            Int32 targetSize = minDim > 6 ? (minDim - 6) : minDim;

            Int32 targetW = targetSize;
            Int32 targetH = targetSize;

            if (icon.w > 0 && icon.h > 0)
            {
                Float aspect = (Float)icon.w / (Float)icon.h;
                if (aspect > 1.05)
                    targetH = (Int32)(targetSize / aspect + 0.5);
                else if (aspect < 0.95)
                    targetW = (Int32)(targetSize * aspect + 0.5);
            }

            Int32 ix = (w - targetW) / 2;
            Int32 iy = (h - targetH) / 2;

            DrawBitmap(icon.bmp, ix, iy, targetW, targetH, icon.x, icon.y, icon.w, icon.h, drawflags);

            if (m_dragHover)
            {
                DrawSetPen(Vector(0.35, 0.70, 1.0));
                DrawBorder(BORDER_ACTIVE_1, 0, 0, w - 1, h - 1);
            }
            else
            {
                DrawBorder(BORDER_THIN_IN, 0, 0, w - 1, h - 1);
            }
            return;
        }
    }

    if (m_dragHover)
    {
        DrawSetPen(Vector(0.35, 0.70, 1.0));
        DrawBorder(BORDER_ACTIVE_1, 0, 0, w - 1, h - 1);
    }
    else
    {
        DrawBorder(BORDER_THIN_IN, 0, 0, w - 1, h - 1);
    }

    // Centered subtle '+' crosshair when empty and enabled
    if (m_enabled)
    {
        Int32 cx = w / 2;
        Int32 cy = h / 2;
        DrawSetPen(m_dragHover ? Vector(0.45, 0.80, 1.0) : Vector(0.32, 0.34, 0.38));
        DrawLine(cx - 5, cy, cx + 5, cy);
        DrawLine(cx, cy - 5, cx, cy + 5);
    }
}

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
    if (col == 'tree')
        return 150;
    if (col == 'cmdi')
        return 65;
    if (col == 'scut')
        return LV_COLUMN_WIDTH_FIT;
    return LV_COLUMN_WIDTH_FIT;
}

Int32 CommandListTreeView::GetHeaderColumnWidth(void* root, void* userdata, Int32 col, GeUserArea* area)
{
    if (col == 'icon')
        return 24;
    if (col == 'tree')
        return 150;
    if (col == 'cmdi')
        return 65;
    if (col == 'scut')
        return LV_COLUMN_WIDTH_FIT;
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
    else if (col == 'scut')
    {
        if (item->shortcut.IsPopulated())
        {
            drawinfo->frame->DrawSetTextCol(Vector(0.72, 0.74, 0.78), COLOR_TRANS);
            Int32 th = drawinfo->frame->DrawGetFontHeight();
            Int32 ty = drawinfo->ypos + (drawinfo->height - th) / 2;
            drawinfo->frame->DrawText(item->shortcut, drawinfo->xpos + 4, ty);
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
// SUBMENU LIST TREE VIEW FUNCTIONS IMPLEMENTATION
// ============================================================================

void* SubmenuListTreeView::GetFirst(void* root, void* userdata)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    if (!dlg)
        return nullptr;
    const maxon::BaseArray<SubmenuItem>& list = dlg->GetAllSubmenus();
    return (list.GetCount() > 0) ? const_cast<SubmenuItem*>(&list[0]) : nullptr;
}

void* SubmenuListTreeView::GetDown(void* root, void* userdata, void* obj)
{
    return nullptr;
}

void* SubmenuListTreeView::GetNext(void* root, void* userdata, void* obj)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    SubmenuItem* item = static_cast<SubmenuItem*>(obj);
    if (!dlg || !item)
        return nullptr;

    const maxon::BaseArray<SubmenuItem>& list = dlg->GetAllSubmenus();
    for (Int32 i = 0; i < (Int32)list.GetCount(); ++i)
    {
        if (&list[i] == item && (i + 1) < (Int32)list.GetCount())
            return const_cast<SubmenuItem*>(&list[i + 1]);
    }
    return nullptr;
}

void* SubmenuListTreeView::GetPred(void* root, void* userdata, void* obj)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    SubmenuItem* item = static_cast<SubmenuItem*>(obj);
    if (!dlg || !item)
        return nullptr;

    const maxon::BaseArray<SubmenuItem>& list = dlg->GetAllSubmenus();
    for (Int32 i = 0; i < (Int32)list.GetCount(); ++i)
    {
        if (&list[i] == item && i > 0)
            return const_cast<SubmenuItem*>(&list[i - 1]);
    }
    return nullptr;
}

Bool SubmenuListTreeView::IsSelected(void* root, void* userdata, void* obj)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    return dlg ? (dlg->GetSelectedSubmenu() == obj) : false;
}

void SubmenuListTreeView::Select(void* root, void* userdata, void* obj, Int32 mode)
{
    PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
    if (!dlg)
        return;

    if (mode == SELECTION_NEW || mode == SELECTION_ADD)
    {
        dlg->SetSelectedSubmenu(static_cast<SubmenuItem*>(obj));
    }
    else if (mode == SELECTION_SUB)
    {
        if (dlg->GetSelectedSubmenu() == obj)
            dlg->SetSelectedSubmenu(nullptr);
    }
}

Bool SubmenuListTreeView::IsOpened(void* root, void* userdata, void* obj)
{
    return false;
}

String SubmenuListTreeView::GetName(void* root, void* userdata, void* obj)
{
    SubmenuItem* item = static_cast<SubmenuItem*>(obj);
    return item ? item->name : String();
}

Int SubmenuListTreeView::GetId(void* root, void* userdata, void* obj)
{
    SubmenuItem* item = static_cast<SubmenuItem*>(obj);
    return item ? (Int)item->index : 0;
}

Int32 SubmenuListTreeView::GetDragType(void* root, void* userdata, void* obj)
{
    return DRAGTYPE_PIEMENU_SUBMENU;
}

Int32 SubmenuListTreeView::DragStart(void* root, void* userdata, void* obj)
{
    return TREEVIEW_DRAGSTART_ALLOW | TREEVIEW_DRAGSTART_SELECT;
}

void SubmenuListTreeView::GenerateDragData(void* root, void* userdata, void* obj, void** dragdata)
{
    SubmenuItem* item = static_cast<SubmenuItem*>(obj);
    if (item && dragdata)
    {
        *dragdata = obj;
    }
}

Int32 SubmenuListTreeView::AcceptDragObject(void* root, void* userdata, void* obj, Int32 dragtype, void* dragobject, Bool& bAllowCopy)
{
    return 0;
}

Int32 SubmenuListTreeView::GetColumnWidth(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area)
{
    if (col == 'tree')
        return LV_COLUMN_WIDTH_FIT;
    if (col == 'ctxt')
        return 80;
    if (col == 'slot')
        return 50;
    return LV_COLUMN_WIDTH_FIT;
}

Int32 SubmenuListTreeView::GetHeaderColumnWidth(void* root, void* userdata, Int32 col, GeUserArea* area)
{
    if (col == 'tree')
        return LV_COLUMN_WIDTH_FIT;
    if (col == 'ctxt')
        return 80;
    if (col == 'slot')
        return 50;
    return LV_COLUMN_WIDTH_FIT;
}

Int32 SubmenuListTreeView::GetLineHeight(void* root, void* userdata, void* obj, Int32 col, GeUserArea* area)
{
    return 22;
}

void SubmenuListTreeView::DrawCell(void* root, void* userdata, void* obj, Int32 col, DrawInfo* drawinfo, const GeData& bgColor)
{
    SubmenuItem* item = static_cast<SubmenuItem*>(obj);
    if (!item || !drawinfo || !drawinfo->frame)
        return;

    if (col == 'ctxt')
    {
        String ctxStr = GetContextShortName(item->context);
        drawinfo->frame->DrawSetTextCol(Vector(0.55, 0.70, 0.90), COLOR_TRANS);
        Int32 th = drawinfo->frame->DrawGetFontHeight();
        Int32 ty = drawinfo->ypos + (drawinfo->height - th) / 2;
        drawinfo->frame->DrawText(ctxStr, drawinfo->xpos + 4, ty);
    }
    else if (col == 'slot')
    {
        String scStr = FormatString("@s"_s, item->slotCount);
        drawinfo->frame->DrawSetTextCol(Vector(0.50, 0.53, 0.58), COLOR_TRANS);
        Int32 th = drawinfo->frame->DrawGetFontHeight();
        Int32 ty = drawinfo->ypos + (drawinfo->height - th) / 2;
        drawinfo->frame->DrawText(scStr, drawinfo->xpos + 4, ty);
    }
}

Int32 SubmenuListTreeView::DoubleClick(void* root, void* userdata, void* obj, Int32 col, MouseInfo* mouseinfo)
{
    SubmenuItem* item = static_cast<SubmenuItem*>(obj);
    if (item && userdata)
    {
        PieMenuEditorDialog* dlg = static_cast<PieMenuEditorDialog*>(userdata);
        dlg->AssignSubmenuToSelectedSlot(item->name);
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

    // Main 2-column layout (Left: Commands/Submenus Library, Right: Pie Menu & Inspector)
    GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 2, 0, ""_s, 0);
    GroupBorderSpace(10, 8, 10, 4);
    GroupSpace(10, 0);

    // =============================================================
    // LEFT COLUMN: Tabs for Commands & Submenus
    // =============================================================
    GroupBegin(IDC_GROUP_CMD_PANEL, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, ""_s, BORDER_GROUP_IN);
    GroupBorderSpace(4, 4, 4, 4);

    TabGroupBegin(IDC_LEFT_TAB_GROUP, BFH_SCALEFIT | BFV_SCALEFIT, TAB_TABS);

    // TAB 1: Commands Library & Search
    GroupBegin(IDC_TAB_COMMANDS, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "C4D Commands"_s, 0);
    GroupBorderSpace(6, 6, 6, 6);
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
    AddStaticText(0, BFH_SCALEFIT, 0, 0, "Drag command to slot or double-click to assign"_s, 0);

    GroupEnd(); // End IDC_TAB_COMMANDS

    // TAB 2: Created Menus (Submenus)
    GroupBegin(IDC_TAB_SUBMENUS, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, "Submenus"_s, 0);
    GroupBorderSpace(6, 6, 6, 6);
    GroupSpace(0, 6);

    GroupBegin(0, BFH_SCALEFIT, 3, 0, ""_s, 0);
    AddStaticText(IDC_SUBMENU_COUNT, BFH_SCALEFIT, 0, 0, "0 submenus available"_s, 0);
    AddButton(IDC_SUBMENU_BTN_ASSIGN, BFH_RIGHT, 0, 0, "Assign as Submenu ->"_s);
    AddButton(IDC_SUBMENU_BTN_NEW, BFH_RIGHT, 0, 0, "+ New Menu"_s);
    GroupEnd();

    BaseContainer subtreedata;
    subtreedata.SetBool(TREEVIEW_BORDER, true);
    subtreedata.SetBool(TREEVIEW_HAS_HEADER, true);
    subtreedata.SetBool(TREEVIEW_HIDE_LINES, true);
    subtreedata.SetBool(TREEVIEW_FIXED_LAYOUT, true);
    subtreedata.SetBool(TREEVIEW_ALTERNATE_BG, true);
    subtreedata.SetBool(TREEVIEW_NO_MULTISELECT, true);
    subtreedata.SetBool(TREEVIEW_CURSORKEYS, true);
    subtreedata.SetBool(TREEVIEW_RESIZE_HEADER, true);
    subtreedata.SetBool(TREEVIEW_DRAG_START_REQUIRES_MOVEMENT, true);

    m_submenuTreeGui = (TreeViewCustomGui*)AddCustomGui(IDC_SUBMENU_TREEVIEW, CUSTOMGUI_TREEVIEW, String(), BFH_SCALEFIT | BFV_SCALEFIT, 300, 400, subtreedata);
    AddStaticText(0, BFH_SCALEFIT, 0, 0, "Drag menu to slot or double-click to link as submenu"_s, 0);

    GroupEnd(); // End IDC_TAB_SUBMENUS

    GroupEnd(); // End IDC_LEFT_TAB_GROUP
    GroupEnd(); // End IDC_GROUP_CMD_PANEL

    // =============================================================
    // RIGHT COLUMN: Pie Preview, Profile & Properties
    // =============================================================
    GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, ""_s, 0);
    GroupSpace(0, 6);

    // 1. Active Menu Properties & Management unified group
    GroupBegin(IDC_GROUP_MENU_SETTINGS, BFH_SCALEFIT, 1, 0, "Menu Properties"_s, BORDER_GROUP_IN);
    GroupBorderSpace(8, 6, 8, 6);
    GroupSpace(0, 4);

    // Section 1: Active Menu, Context, Slots, Name & Title
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    GroupSpace(6, 4);

    // Row 1: Active Menu selection and management buttons
    AddStaticText(0, BFH_LEFT, 0, 0, "Active Menu:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddComboBox(IDC_PROFILE_COMBO, BFH_SCALEFIT, 200, 0);
    GroupBegin(0, BFH_RIGHT, 3, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddButton(IDC_BTN_NEW_PROFILE, BFH_LEFT, 75, 0, "+ New Menu"_s);
    AddButton(IDC_BTN_DUPLICATE_PROFILE, BFH_LEFT, 75, 0, "Duplicate"_s);
    AddButton(IDC_BTN_DELETE_PROFILE, BFH_LEFT, 75, 0, "- Delete"_s);
    GroupEnd();
    GroupEnd();

    // Row 2: Context selection immediately below Active Menu
    AddStaticText(0, BFH_LEFT, 0, 0, "Context:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddComboBox(IDC_PROFILE_CONTEXT_COMBO, BFH_SCALEFIT, 200, 0);
    GroupBegin(0, BFH_RIGHT, 3, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddStaticText(0, BFH_LEFT, 75, 0, ""_s, 0);
    AddStaticText(0, BFH_LEFT, 75, 0, ""_s, 0);
    AddStaticText(0, BFH_LEFT, 75, 0, ""_s, 0);
    GroupEnd();
    GroupEnd();

    // Row 3: Name & Show Title / Show Item Name
    AddStaticText(0, BFH_LEFT, 0, 0, "Name:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddEditText(IDC_PROFILE_NAME_EDIT, BFH_SCALEFIT, 160, 0);
    GroupBegin(0, BFH_RIGHT, 2, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddCheckbox(IDC_PROFILE_SHOW_TITLE, BFH_LEFT, 0, 0, "Show Title"_s);
    AddCheckbox(IDC_PROFILE_SHOW_ITEM_NAME, BFH_LEFT, 0, 0, "Show Item Name"_s);
    GroupEnd();
    GroupEnd();

    // Row 4: Slots count compact number box + stretched slider
    AddStaticText(0, BFH_LEFT, 0, 0, "Slots (2-24):"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddEditNumber(IDC_SLOTCOUNT_NUMBER, BFH_LEFT, 50, 0);
    AddSlider(IDC_SLOTCOUNT_SLIDER, BFH_SCALEFIT, 0, 0);
    GroupEnd();

    GroupEnd();

    // Section 2: Display style, opacity and sizing sliders
    GroupBegin(0, BFH_SCALEFIT, 4, 0, ""_s, 0);
    GroupSpace(6, 4);

    // Row 5: Display Style and Opacity
    AddStaticText(0, BFH_LEFT, 0, 0, "Display Style:"_s, 0);
    AddComboBox(IDC_GLOBAL_DISPLAY_MODE, BFH_SCALEFIT, 110, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Opacity:"_s, 0);
    AddEditSlider(IDC_GLOBAL_OPACITY, BFH_SCALEFIT, 110, 0);

    // Row 5: Outer & Inner Radii
    AddStaticText(0, BFH_LEFT, 0, 0, "Outer Radius:"_s, 0);
    AddEditSlider(IDC_GLOBAL_RADIUS, BFH_SCALEFIT, 110, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Inner Radius:"_s, 0);
    AddEditSlider(IDC_GLOBAL_INNER_RADIUS, BFH_SCALEFIT, 110, 0);

    // Row 6: Deadzone & Icon Size
    AddStaticText(0, BFH_LEFT, 0, 0, "Deadzone:"_s, 0);
    AddEditSlider(IDC_GLOBAL_DEADZONE, BFH_SCALEFIT, 110, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Icon Size:"_s, 0);
    AddEditSlider(IDC_GLOBAL_ICON_SIZE, BFH_SCALEFIT, 110, 0);

    GroupEnd();

    GroupEnd();

    // 4. Menu Hotkey settings group for the active profile
    GroupBegin(IDC_GROUP_MENU_HOTKEY, BFH_SCALEFIT, 1, 0, "Menu Hotkey (Trigger Shortcut)"_s, BORDER_GROUP_IN);
    GroupBorderSpace(8, 6, 8, 6);
    GroupSpace(0, 4);

    // Row 1: Hotkey modifiers and key selection
    GroupBegin(0, BFH_SCALEFIT, 8, 0, ""_s, 0);
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

    // Row 2: Behavior on key release
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    GroupSpace(16, 0);
    AddCheckbox(IDC_PROFILE_STAY_OPEN_ON_RELEASE, BFH_LEFT, 0, 0, "Stay Open on Key Release"_s);
    AddCheckbox(IDC_PROFILE_STAY_OPEN_CENTER, BFH_LEFT, 0, 0, "Stay Open with Center Command"_s);
    GroupEnd();

    GroupEnd();

    // UserArea preview (real size)
    GroupBegin(0, BFH_SCALEFIT | BFV_SCALEFIT, 1, 0, ""_s, 0);
    GroupBorderSpace(4, 0, 4, 4);
    AddUserArea(IDC_USERAREA_PIE, BFH_SCALEFIT | BFV_SCALEFIT, 360, 320);
    AttachUserArea(m_userArea, IDC_USERAREA_PIE);
    GroupEnd();

    // Selected Slot Inspector group
    GroupBegin(IDC_GROUP_INSPECTOR, BFH_SCALEFIT, 2, 0, "Selected Slot Properties"_s, BORDER_GROUP_IN);
    GroupBorderSpace(10, 6, 10, 6);

    AddStaticText(0, BFH_LEFT, 0, 0, "Enabled:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 3, 0, ""_s, 0);
    GroupSpace(8, 0);
    AddCheckbox(IDC_INSPECTOR_ENABLED, BFH_LEFT, 0, 0, "Active in Menu"_s);
    AddStaticText(0, BFH_LEFT, 0, 0, "Accent Color:"_s, 0);
    AddColorField(IDC_INSPECTOR_COLOR_FIELD, BFH_LEFT, 60, 18);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Submenu Link:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 2, 0, ""_s, 0);
    GroupSpace(6, 0);
    AddComboBox(IDC_INSPECTOR_SUBMENU_COMBO, BFH_SCALEFIT, 150, 0);
    AddButton(IDC_INSPECTOR_GOTO_SUBMENU, BFH_LEFT, 80, 0, "Edit Menu ->"_s);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Main Action:"_s, 0);
    GroupBegin(IDC_GROUP_MAIN_ACTION, BFH_SCALEFIT, 5, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddUserArea(IDC_INSPECTOR_MAIN_ICON, BFH_LEFT | BFV_CENTER, 34, 34);
    AttachUserArea(m_mainIconArea, IDC_INSPECTOR_MAIN_ICON);
    AddEditText(IDC_INSPECTOR_CMD_NAME, BFH_SCALEFIT, 120, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "ID:"_s, 0);
    AddEditNumber(IDC_INSPECTOR_CMD_ID, BFH_LEFT, 65, 0);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Preset:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 4, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddComboBox(IDC_INSPECTOR_PRESET_COMBO, BFH_SCALEFIT, 130, 0);
    AddButton(IDC_INSPECTOR_PRESET_CAPTURE_BTN, BFH_LEFT, 70, 0, "Snapshot"_s);
    AddButton(IDC_INSPECTOR_PRESET_CLEAR_BTN, BFH_LEFT, 45, 0, "Reset"_s);
    AddStaticText(IDC_INSPECTOR_PRESET_STATUS_TEXT, BFH_LEFT, 55, 0, ""_s, 0);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Shift Action:"_s, 0);
    GroupBegin(IDC_GROUP_SHIFT_ACTION, BFH_SCALEFIT, 7, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddUserArea(IDC_INSPECTOR_SHIFT_ICON, BFH_LEFT | BFV_CENTER, 34, 34);
    AttachUserArea(m_shiftIconArea, IDC_INSPECTOR_SHIFT_ICON);
    AddEditText(IDC_INSPECTOR_SHIFT_NAME, BFH_SCALEFIT, 100, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "ID:"_s, 0);
    AddEditNumber(IDC_INSPECTOR_SHIFT_ID, BFH_LEFT, 65, 0);
    AddButton(IDC_INSPECTOR_SHIFT_SET, BFH_LEFT, 55, 0, "Set Sel"_s);
    AddButton(IDC_INSPECTOR_SHIFT_CLEAR, BFH_LEFT, 45, 0, "Reset"_s);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Shift Preset:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 4, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddComboBox(IDC_INSPECTOR_SHIFT_PRESET_COMBO, BFH_SCALEFIT, 130, 0);
    AddButton(IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN, BFH_LEFT, 70, 0, "Snapshot"_s);
    AddButton(IDC_INSPECTOR_SHIFT_PRESET_CLEAR_BTN, BFH_LEFT, 45, 0, "Reset"_s);
    AddStaticText(IDC_INSPECTOR_SHIFT_PRESET_STATUS_TEXT, BFH_LEFT, 55, 0, ""_s, 0);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Ctrl Action:"_s, 0);
    GroupBegin(IDC_GROUP_CTRL_ACTION, BFH_SCALEFIT, 7, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddUserArea(IDC_INSPECTOR_CTRL_ICON, BFH_LEFT | BFV_CENTER, 34, 34);
    AttachUserArea(m_ctrlIconArea, IDC_INSPECTOR_CTRL_ICON);
    AddEditText(IDC_INSPECTOR_CTRL_NAME, BFH_SCALEFIT, 100, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "ID:"_s, 0);
    AddEditNumber(IDC_INSPECTOR_CTRL_ID, BFH_LEFT, 65, 0);
    AddButton(IDC_INSPECTOR_CTRL_SET, BFH_LEFT, 55, 0, "Set Sel"_s);
    AddButton(IDC_INSPECTOR_CTRL_CLEAR, BFH_LEFT, 45, 0, "Reset"_s);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Ctrl Preset:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 4, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddComboBox(IDC_INSPECTOR_CTRL_PRESET_COMBO, BFH_SCALEFIT, 130, 0);
    AddButton(IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN, BFH_LEFT, 70, 0, "Snapshot"_s);
    AddButton(IDC_INSPECTOR_CTRL_PRESET_CLEAR_BTN, BFH_LEFT, 45, 0, "Reset"_s);
    AddStaticText(IDC_INSPECTOR_CTRL_PRESET_STATUS_TEXT, BFH_LEFT, 55, 0, ""_s, 0);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Alt Action:"_s, 0);
    GroupBegin(IDC_GROUP_ALT_ACTION, BFH_SCALEFIT, 7, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddUserArea(IDC_INSPECTOR_ALT_ICON, BFH_LEFT | BFV_CENTER, 34, 34);
    AttachUserArea(m_altIconArea, IDC_INSPECTOR_ALT_ICON);
    AddEditText(IDC_INSPECTOR_ALT_NAME, BFH_SCALEFIT, 100, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "ID:"_s, 0);
    AddEditNumber(IDC_INSPECTOR_ALT_ID, BFH_LEFT, 65, 0);
    AddButton(IDC_INSPECTOR_ALT_SET, BFH_LEFT, 55, 0, "Set Sel"_s);
    AddButton(IDC_INSPECTOR_ALT_CLEAR, BFH_LEFT, 45, 0, "Reset"_s);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, "Alt Preset:"_s, 0);
    GroupBegin(0, BFH_SCALEFIT, 4, 0, ""_s, 0);
    GroupSpace(4, 0);
    AddComboBox(IDC_INSPECTOR_ALT_PRESET_COMBO, BFH_SCALEFIT, 130, 0);
    AddButton(IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN, BFH_LEFT, 70, 0, "Snapshot"_s);
    AddButton(IDC_INSPECTOR_ALT_PRESET_CLEAR_BTN, BFH_LEFT, 45, 0, "Reset"_s);
    AddStaticText(IDC_INSPECTOR_ALT_PRESET_STATUS_TEXT, BFH_LEFT, 55, 0, ""_s, 0);
    GroupEnd();

    AddStaticText(0, BFH_LEFT, 0, 0, ""_s, 0);
    AddButton(IDC_INSPECTOR_CLEAR_SLOT, BFH_LEFT, 110, 0, "Clear Slot"_s);

    GroupEnd();

    GroupEnd(); // End Right Column

    GroupEnd(); // End Main 2-column layout

    // 3. Action buttons at bottom
    GroupBegin(0, BFH_SCALEFIT, 4, 0, ""_s, 0);
    GroupBorderSpace(12, 6, 12, 10);
    AddButton(IDC_BTN_SAVE, BFH_SCALEFIT, 0, 0, "Save Configuration"_s);
    AddButton(IDC_BTN_EXPORT, BFH_SCALEFIT, 0, 0, "Export Settings..."_s);
    AddButton(IDC_BTN_IMPORT, BFH_SCALEFIT, 0, 0, "Import Settings..."_s);
    AddButton(IDC_BTN_CLOSE, BFH_SCALEFIT, 0, 0, "Close"_s);
    GroupEnd();

    return true;
}

void PieMenuEditorDialog::PopulateAllCommands()
{
    if (m_allCommands.GetCount() > 0)
        return;

    std::unordered_set<Int32> seen;

    // 0. Pre-cache all shortcut mappings from Cinema 4D
    std::unordered_map<Int32, String> shortcutMap;
    const Int32 scCount = GetShortcutCount();
    for (Int32 i = 0; i < scCount; ++i)
    {
        BaseContainer sc = GetShortcut(i);
        Int32 cmdId = sc.GetInt32(SHORTCUT_PLUGINID);
        if (cmdId <= 0)
            continue;

        String scStr = Shortcut2String(sc);
        if (!scStr.IsPopulated())
            continue;

        auto it = shortcutMap.find(cmdId);
        if (it == shortcutMap.end())
        {
            shortcutMap[cmdId] = scStr;
        }
        else
        {
            if (!it->second.Find(scStr, nullptr))
            {
                it->second += ", "_s;
                it->second += scStr;
            }
        }
    }

    auto assignShortcut = [&](CommandItem& item)
    {
        auto it = shortcutMap.find(item.id);
        if (it != shortcutMap.end())
            item.shortcut = it->second;
    };

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
            assignShortcut(item);
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
            assignShortcut(item);
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
                        assignShortcut(item);
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

    // 4. Ensure any active commands with shortcuts are included
    for (const auto& pair : shortcutMap)
    {
        Int32 id = pair.first;
        if (id > 0 && seen.find(id) == seen.end())
        {
            String name = GetCommandName(id);
            if (name.IsPopulated())
            {
                seen.insert(id);
                CommandItem item;
                item.id = id;
                item.name = name;
                item.shortcut = pair.second;
                item.hasIcon = GetIcon(id, &item.icon) && (item.icon.bmp != nullptr);
                m_allCommands.Append(item) iferr_ignore("Append");
            }
        }
    }

    // Sort alphabetically by name
    std::sort(m_allCommands.Begin(), m_allCommands.End(), [](const CommandItem& a, const CommandItem& b)
    {
        return a.name.Compare(b.name, maxon::STRINGCOMPARISON::CASEINSENSITIVE_NUMBERS) == maxon::COMPARERESULT::LESS;
    });
}

static wchar_t MapRuCharToEn(wchar_t ch)
{
    // Russian lowercase 'а' (0x0430) to 'я' (0x044F) mapped to QWERTY
    static const wchar_t s_ruMap[] = {
        L'f', L',', L'd', L'u', L'l', L't', L';', L'p', // а, б, в, г, д, е, ж, з
        L'b', L'q', L'r', L'k', L'v', L'y', L'j', L'g', // и, й, к, л, м, н, о, п
        L'h', L'c', L'n', L'e', L'a', L'[', L'w', L'x', // р, с, т, у, ф, х, ц, ч
        L'i', L'o', L']', L's', L'm', L'\'', L'.', L'z'  // ш, щ, ъ, ы, ь, э, ю, я
    };
    if (ch >= 0x0430 && ch <= 0x044F)
        return s_ruMap[ch - 0x0430];
    if (ch == 0x0451) // ё
        return L'`';
    return ch;
}

static String ConvertRuToEnLayout(const String& str)
{
    Char* cstr = str.GetCStringCopy(STRINGENCODING::UTF8);
    if (!cstr) return String();
    int req = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, nullptr, 0);
    if (req <= 1) { DeleteMem(cstr); return String(); }
    std::vector<wchar_t> wbuf(req);
    MultiByteToWideChar(CP_UTF8, 0, cstr, -1, wbuf.data(), req);
    DeleteMem(cstr);

    bool changed = false;
    for (size_t i = 0; i < wbuf.size(); ++i)
    {
        wchar_t mapped = MapRuCharToEn(wbuf[i]);
        if (mapped != wbuf[i])
        {
            wbuf[i] = mapped;
            changed = true;
        }
    }
    if (!changed)
        return String();

    int reqMb = WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, nullptr, 0, nullptr, nullptr);
    if (reqMb <= 1) return String();
    std::vector<char> mbbuf(reqMb);
    WideCharToMultiByte(CP_UTF8, 0, wbuf.data(), -1, mbbuf.data(), reqMb, nullptr, nullptr);
    return String(mbbuf.data());
}

static Bool ShortcutMatchesQuery(const String& shortcutStr, const String& q, const String& qAlt)
{
    if (shortcutStr.IsEmpty() || q.IsEmpty())
        return false;

    auto testQuery = [&](const String& query) -> Bool
    {
        if (query.IsEmpty())
            return false;

        String qLow = query.ToLower();
        String scLow = shortcutStr.ToLower();

        // Check if query specifies a modifier or chord sequence
        Bool isModifierQuery = (qLow.Find("ctrl"_s, nullptr) ||
                                qLow.Find("shift"_s, nullptr) ||
                                qLow.Find("alt"_s, nullptr) ||
                                qLow.Find("cmd"_s, nullptr) ||
                                qLow.Find("+"_s, nullptr) ||
                                qLow.Find("~"_s, nullptr));

        if (isModifierQuery)
        {
            // Direct substring match (e.g. "ctrl+e", "shift", "u~l", "u~")
            if (scLow.Find(qLow, nullptr))
                return true;

            // Compact match without spaces (e.g. "ctrl + e" matches "ctrl+e")
            String qCompact = qLow;
            qCompact.Replace(' ', ""_s) iferr_ignore("Replace");
            String scCompact = scLow;
            scCompact.Replace(' ', ""_s) iferr_ignore("Replace");
            if (scCompact.Find(qCompact, nullptr))
                return true;

            // Space-to-plus match (e.g. "ctrl e" matches "ctrl+e")
            String qWithPlus = qLow;
            qWithPlus.Replace(' ', "+"_s) iferr_ignore("Replace");
            if (scLow.Find(qWithPlus, nullptr))
                return true;

            // Chords with space instead of tilde (e.g. "u l" matches "u~l")
            String scTildeToSpace = scLow;
            scTildeToSpace.Replace('~', ' ') iferr_ignore("Replace");
            if (scTildeToSpace.Find(qLow, nullptr))
                return true;

            return false;
        }

        // For non-modifier queries, test individual shortcut tokens (split by ", ")
        // so that single keys like "e", "f1", "space" don't match modifier names ("shift", "ctrl")
        Bool tokenMatch = false;
        scLow.Split(","_s, true, [&](const maxon::String& part) -> maxon::Result<Bool>
        {
            if (part == qLow)
            {
                tokenMatch = true;
                return false; // stop iteration
            }
            if (qLow.GetLength() > 2 && part.Find(qLow, nullptr))
            {
                tokenMatch = true;
                return false;
            }
            return true; // continue iteration
        }) iferr_ignore("Split");

        if (tokenMatch)
            return true;

        return false;
    };

    if (testQuery(q))
        return true;

    if (qAlt.IsPopulated() && qAlt != q && testQuery(qAlt))
        return true;

    return false;
}

void PieMenuEditorDialog::FilterCommands(const String& query)
{
    m_filteredCommands.Reset();
    String q = query;
    q = q.ToLower();

    Bool shortcutOnly = false;
    if (q.StartsWith('@') || q.StartsWith('#'))
    {
        shortcutOnly = true;
        q = q.GetRightPart(1);
        q.Trim() iferr_ignore("Trim");
    }

    String qAlt = ConvertRuToEnLayout(q);
    Bool hasAlt = (qAlt.IsPopulated() && qAlt != q);

    for (CommandItem& item : m_allCommands)
    {
        Bool match = false;
        if (q.IsEmpty())
        {
            match = true;
        }
        else if (shortcutOnly)
        {
            if (ShortcutMatchesQuery(item.shortcut, q, qAlt))
                match = true;
        }
        else
        {
            if (item.name.ToLower().Find(q, nullptr) || (hasAlt && item.name.ToLower().Find(qAlt, nullptr)))
            {
                match = true;
            }
            else if (ShortcutMatchesQuery(item.shortcut, q, qAlt))
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
    if (sel != CENTER_SLOT_INDEX && (sel < 0 || sel >= prof->slotCount))
        sel = 0;

    if (prof->slots[sel].commandId != cmdId)
        prof->slots[sel].ClearToolPreset();

    prof->slots[sel].commandId = cmdId;
    prof->slots[sel].subMenuName = ""_s;
    prof->slots[sel].customLabel = GetCommandName(cmdId);
    prof->slots[sel].enabled = true;

    UpdateInspector();
    m_userArea.Redraw();
}

void PieMenuEditorDialog::AssignSubmenuToSelectedSlot(const maxon::String& subMenuName)
{
    if (subMenuName.IsEmpty())
        return;

    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    Int32 sel = m_userArea.GetSelectedSlot();
    if (sel != CENTER_SLOT_INDEX && (sel < 0 || sel >= prof->slotCount))
        sel = 0;

    prof->slots[sel].ClearToolPreset();
    prof->slots[sel].subMenuName = subMenuName;
    prof->slots[sel].commandId = NOTOK;
    prof->slots[sel].customLabel = subMenuName;
    prof->slots[sel].enabled = true;

    UpdateInspector();
    m_userArea.Redraw();
}

void PieMenuEditorDialog::PopulateAllSubmenus()
{
    m_allSubmenus.Reset();
    PieProfileManager& mgr = PieProfileManager::Get();
    Int32 activeIdx = m_userArea.GetActiveProfileIndex();

    for (Int32 i = 0; i < mgr.GetProfileCount(); ++i)
    {
        if (i == activeIdx)
            continue; // Cannot add active menu to itself

        PieProfile* prof = mgr.GetProfile(i);
        if (prof)
        {
            SubmenuItem item;
            item.index = i;
            item.name = prof->name;
            item.context = prof->context;
            item.slotCount = prof->slotCount;
            item.hotkeyStr = prof->hotkey.GetDisplayString();
            m_allSubmenus.Append(item) iferr_ignore("Append");
        }
    }
}

void PieMenuEditorDialog::RefreshSubmenuList()
{
    PopulateAllSubmenus();

    if (m_submenuTreeGui)
    {
        m_submenuTreeGui->Refresh();
    }

    String countStr = FormatString("@ menu(s) available"_s, (Int32)m_allSubmenus.GetCount());
    SetString(IDC_SUBMENU_COUNT, countStr);

    // Populate Inspector submenu dropdown
    FreeChildren(IDC_INSPECTOR_SUBMENU_COMBO);
    AddChild(IDC_INSPECTOR_SUBMENU_COMBO, 0, "-- None (Execute Command) --"_s);
    for (Int32 i = 0; i < (Int32)m_allSubmenus.GetCount(); ++i)
    {
        String title = FormatString("Submenu: @"_s, m_allSubmenus[i].name);
        AddChild(IDC_INSPECTOR_SUBMENU_COMBO, i + 1, title);
    }
}

static Bool CapturePresetSnapshot(BaseDocument* doc, Int32 targetCmdId, BaseContainer& outData, String& outName)
{
    if (!doc || targetCmdId <= 0)
        return false;

    // 1. Try tool data first if it's a tool
    BaseContainer* toolBc = GetToolData(doc, targetCmdId, true);
    if (!toolBc)
        toolBc = GetToolData(doc, targetCmdId, false);
    if (toolBc && !toolBc->IsEmpty())
    {
        outData = *toolBc;
        outName = "Tool Settings"_s;
        return true;
    }

    // 2. Try active object if type matches targetCmdId
    BaseObject* activeObj = doc->GetActiveObject();
    if (activeObj && activeObj->GetType() == targetCmdId)
    {
        outData = activeObj->GetData();
        outName = activeObj->GetName();
        return true;
    }

    // 3. Fallback: instantiate temporary default object if it's an object generator/primitive
    BaseObject* tempObj = BaseObject::Alloc(targetCmdId);
    if (tempObj)
    {
        outData = tempObj->GetData();
        outName = "Object Defaults"_s;
        BaseObject::Free(tempObj);
        return true;
    }

    return false;
}

static void HandlePresetComboChange(
    Int32 comboVal,
    ToolPresetMode& outMode,
    maxon::String& outPresetName,
    maxon::String& outAssetId,
    const BaseContainer& settings,
    const maxon::BaseArray<PieMenuEditorDialog::DiscoveredPresetItem>& discoveredPresets)
{
    if (comboVal == 0)
    {
        outMode = ToolPresetMode::None;
        outAssetId = ""_s;
    }
    else
    {
        Int32 snapComboId = !settings.IsEmpty() ? 1 : -1;
        if (snapComboId > 0 && comboVal == snapComboId)
        {
            outMode = ToolPresetMode::Snapshot;
            outAssetId = ""_s;
            if (!outPresetName.IsPopulated())
                outPresetName = "Active Settings"_s;
        }
        else
        {
            for (Int32 i = 0; i < (Int32)discoveredPresets.GetCount(); ++i)
            {
                const PieMenuEditorDialog::DiscoveredPresetItem& dp = discoveredPresets[i];
                if (dp.comboId == comboVal)
                {
                    outMode = ToolPresetMode::Asset;
                    outAssetId = dp.assetId;
                    outPresetName = dp.name;
                    break;
                }
            }
        }
    }
}

void PieMenuEditorDialog::PopulatePresetsForCommand(Int32 cmdId, maxon::BaseArray<DiscoveredPresetItem>& outPresets)
{
    outPresets.Reset();
    if (cmdId <= 0)
    {
        return;
    }

    maxon::AssetRepositoryRef repo = maxon::AssetInterface::GetUserPrefsRepository();
    if (!repo)
    {
        return;
    }

    maxon::String targetSub = FormatString("_@"_s, cmdId);

    repo.FindAssets(maxon::AssetTypes::DefaultsPreset(), maxon::Id(), maxon::Id(), maxon::ASSET_FIND_MODE::LATEST,
        [&outPresets, &targetSub](const maxon::AssetDescription& asset) -> maxon::Result<maxon::Bool> {
            iferr_scope;
            maxon::Id subTypeId = asset.GetMetaData().Get(maxon::ASSETMETADATA::SubType, maxon::Id()) iferr_ignore("Get");
            maxon::String subTypeStr = subTypeId.ToString();

            if (subTypeStr.Find(targetSub, nullptr))
            {
                maxon::String name = asset.GetMetaString(maxon::OBJECT::BASE::NAME) iferr_ignore("GetMetaString");
                if (name.IsEmpty())
                    name = asset.GetId().ToString();

                DiscoveredPresetItem item;
                item.name = name;
                item.assetId = asset.GetId().ToString();
                outPresets.Append(item) iferr_ignore("Append");
            }
            return true;
        }) iferr_ignore("FindAssets");
}

void PieMenuEditorDialog::PopulatePresetsForTool(Int32 cmdId)
{
    PopulatePresetsForCommand(cmdId, m_currentDiscoveredPresets);
}

void PieMenuEditorDialog::UpdatePresetRowUI(
    Int32 comboId, Int32 captureBtnId, Int32 clearBtnId, Int32 statusTextId,
    Int32 targetCmdId,
    ToolPresetMode mode,
    const maxon::String& presetName,
    const maxon::String& presetAssetId,
    const BaseContainer& settings,
    maxon::BaseArray<DiscoveredPresetItem>& discoveredPresets,
    Bool isSlotValid)
{
    FreeChildren(comboId);
    AddChild(comboId, 0, "[Default]"_s);

    if (targetCmdId <= 0)
    {
        SetInt32(comboId, 0);
        Enable(comboId, false);
        Enable(captureBtnId, false);
        Enable(clearBtnId, false);
        SetString(statusTextId, ""_s);
        return;
    }

    PopulatePresetsForCommand(targetCmdId, discoveredPresets);

    Int32 currentComboId = 0;
    Int32 nextComboId = 1;

    Bool hasSnap = !settings.IsEmpty();
    if (hasSnap)
    {
        Int32 snapId = nextComboId++;
        String snapTitle = "[Snapshot: Active Settings]"_s;
        if (presetName.IsPopulated())
            snapTitle = FormatString("[Snapshot: @]"_s, presetName);
        AddChild(comboId, snapId, snapTitle);
        if (mode == ToolPresetMode::Snapshot)
            currentComboId = snapId;
    }

    for (Int32 i = 0; i < (Int32)discoveredPresets.GetCount(); ++i)
    {
        DiscoveredPresetItem& item = discoveredPresets[i];
        item.comboId = nextComboId++;
        AddChild(comboId, item.comboId, item.name);

        if (mode == ToolPresetMode::Asset && presetAssetId == item.assetId)
        {
            currentComboId = item.comboId;
        }
    }

    if (mode == ToolPresetMode::Asset && currentComboId == 0 && presetAssetId.IsPopulated())
    {
        Int32 extraId = nextComboId++;
        String extraTitle = presetName.IsPopulated() ? presetName : "Asset Preset"_s;
        AddChild(comboId, extraId, extraTitle);
        currentComboId = extraId;
    }

    SetInt32(comboId, currentComboId);
    Enable(comboId, isSlotValid);
    Enable(captureBtnId, isSlotValid);
    Enable(clearBtnId, isSlotValid && (mode != ToolPresetMode::None || hasSnap));

    if (mode == ToolPresetMode::Snapshot)
        SetString(statusTextId, "Snapshot"_s);
    else if (mode == ToolPresetMode::Asset)
        SetString(statusTextId, "Asset"_s);
    else
        SetString(statusTextId, hasSnap ? "Default (Saved Snap)"_s : "Default"_s);
}

Bool PieMenuEditorDialog::InitValues()
{
    // Populate key selection combo
    FreeChildren(IDC_HOTKEY_KEY_COMBO);
    for (Int32 i = 0; i < s_availableKeyCount; ++i)
    {
        AddChild(IDC_HOTKEY_KEY_COMBO, i, String(s_availableKeys[i].name));
    }

    // Populate context combo
    FreeChildren(IDC_PROFILE_CONTEXT_COMBO);
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Global, GetContextName(PieContextMode::Global));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Components, GetContextName(PieContextMode::Components));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Polygons, GetContextName(PieContextMode::Polygons));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Edges, GetContextName(PieContextMode::Edges));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Points, GetContextName(PieContextMode::Points));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Objects, GetContextName(PieContextMode::Objects));
    AddChild(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::UV, GetContextName(PieContextMode::UV));


    FreeChildren(IDC_GLOBAL_DISPLAY_MODE);
    AddChild(IDC_GLOBAL_DISPLAY_MODE, 0, "Icon and Label"_s);
    AddChild(IDC_GLOBAL_DISPLAY_MODE, 1, "Icon Only"_s);
    AddChild(IDC_GLOBAL_DISPLAY_MODE, 2, "Label Only"_s);

    // Populate and initialize commands library
    PopulateAllCommands();
    FilterCommands(""_s);

    if (m_treeGui)
    {
        BaseContainer layout;
        layout.SetInt32('icon', LV_USER);
        layout.SetInt32('tree', LV_TREE);
        layout.SetInt32('scut', LV_USER);
        layout.SetInt32('cmdi', LV_USER);
        m_treeGui->SetLayout(4, layout);
        m_treeGui->SetHeaderText('icon', ""_s);
        m_treeGui->SetHeaderText('tree', "Command"_s);
        m_treeGui->SetHeaderText('scut', "Shortcut"_s);
        m_treeGui->SetHeaderText('cmdi', "ID"_s);

        m_treeGui->SetRoot(this, &m_treeFunctions, this);
        m_treeGui->SetColumnWidthUser('icon', 24);
        m_treeGui->SetColumnWidthUser('tree', 150);
        m_treeGui->SetColumnWidthUser('cmdi', 65);
        m_treeGui->Refresh();
    }

    if (m_submenuTreeGui)
    {
        BaseContainer sublayout;
        sublayout.SetInt32('tree', LV_TREE);
        sublayout.SetInt32('ctxt', LV_USER);
        sublayout.SetInt32('slot', LV_USER);
        m_submenuTreeGui->SetLayout(3, sublayout);
        m_submenuTreeGui->SetHeaderText('tree', "Menu Name"_s);
        m_submenuTreeGui->SetHeaderText('ctxt', "Context"_s);
        m_submenuTreeGui->SetHeaderText('slot', "Slots"_s);

        m_submenuTreeGui->SetRoot(this, &m_submenuTreeFunctions, this);
        m_submenuTreeGui->Refresh();
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
    {
        SetString(IDC_PROFILE_NAME_EDIT, ""_s);
        SetInt32(IDC_PROFILE_CONTEXT_COMBO, (Int32)PieContextMode::Global);
        SetInt32(IDC_SLOTCOUNT_NUMBER, 8, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
        SetInt32(IDC_SLOTCOUNT_SLIDER, 8, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
        SetBool(IDC_PROFILE_SHOW_TITLE, true);
        SetBool(IDC_PROFILE_SHOW_ITEM_NAME, true);
        SetBool(IDC_PROFILE_STAY_OPEN_ON_RELEASE, false);
        SetBool(IDC_PROFILE_STAY_OPEN_CENTER, false);
        Enable(IDC_PROFILE_STAY_OPEN_CENTER, false);
        SetPercent(IDC_GLOBAL_OPACITY, 1.0, 10.0, 100.0, 1.0);
        Enable(IDC_GLOBAL_OPACITY, false);
        SetInt32(IDC_GLOBAL_DISPLAY_MODE, (Int32)PieDisplayMode::IconOnly);
        return;
    }

    SetString(IDC_PROFILE_NAME_EDIT, prof->name);
    SetInt32(IDC_PROFILE_CONTEXT_COMBO, (Int32)prof->context);
    SetInt32(IDC_SLOTCOUNT_NUMBER, prof->slotCount, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
    SetInt32(IDC_SLOTCOUNT_SLIDER, prof->slotCount, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
    SetBool(IDC_PROFILE_SHOW_TITLE, prof->showTitle);
    SetBool(IDC_PROFILE_SHOW_ITEM_NAME, prof->showItemName);
    SetBool(IDC_PROFILE_STAY_OPEN_ON_RELEASE, prof->stayOpenOnRelease);
    SetBool(IDC_PROFILE_STAY_OPEN_CENTER, prof->stayOpenWithCenterCommand);
    Enable(IDC_PROFILE_STAY_OPEN_CENTER, prof->stayOpenOnRelease);
    Enable(IDC_GLOBAL_OPACITY, true);

    SetFloat(IDC_GLOBAL_RADIUS, prof->outerRadius, 40.0, 300.0, 1.0);
    SetFloat(IDC_GLOBAL_INNER_RADIUS, prof->innerRadius, 10.0, 200.0, 1.0);
    SetFloat(IDC_GLOBAL_DEADZONE, prof->deadzone, 10.0, 80.0, 1.0);
    SetFloat(IDC_GLOBAL_ICON_SIZE, prof->iconSize, 12.0, 64.0, 1.0);
    SetPercent(IDC_GLOBAL_OPACITY, (prof->opacity >= 0.05) ? prof->opacity : 1.0, 10.0, 100.0, 1.0);
    SetInt32(IDC_GLOBAL_DISPLAY_MODE, (Int32)prof->displayMode);
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

    Int32 sc = 8;
    if (GetInt32(IDC_SLOTCOUNT_SLIDER, sc) || GetInt32(IDC_SLOTCOUNT_NUMBER, sc))
    {
        if (sc < MIN_SLOT_COUNT) sc = MIN_SLOT_COUNT;
        if (sc > MAX_SLOT_COUNT) sc = MAX_SLOT_COUNT;
        prof->slotCount = sc;
    }

    Bool st = true;
    if (GetBool(IDC_PROFILE_SHOW_TITLE, st))
        prof->showTitle = st;

    Bool sin = true;
    if (GetBool(IDC_PROFILE_SHOW_ITEM_NAME, sin))
        prof->showItemName = sin;

    Bool so = false;
    if (GetBool(IDC_PROFILE_STAY_OPEN_ON_RELEASE, so))
    {
        prof->stayOpenOnRelease = so;
        mgr.m_stayOpenOnRelease = so;
    }

    Bool soc = false;
    if (GetBool(IDC_PROFILE_STAY_OPEN_CENTER, soc))
    {
        prof->stayOpenWithCenterCommand = soc;
        mgr.m_stayOpenWithCenterCommand = soc;
    }

    Float outR = 112.0;
    if (GetFloat(IDC_GLOBAL_RADIUS, outR))
    {
        if (outR < 30.0) outR = 30.0;
        if (outR > 300.0) outR = 300.0;
        prof->outerRadius = outR;
        prof->radius = outR;
        mgr.m_outerRadius = outR;
        mgr.m_radius = outR;
    }

    Float inR = 36.0;
    if (GetFloat(IDC_GLOBAL_INNER_RADIUS, inR))
    {
        if (inR < 10.0) inR = 10.0;
        if (inR > 250.0) inR = 250.0;
        prof->innerRadius = inR;
        mgr.m_innerRadius = inR;
    }

    Float dz = 26.0;
    if (GetFloat(IDC_GLOBAL_DEADZONE, dz))
    {
        if (dz < 5.0) dz = 5.0;
        if (dz > 120.0) dz = 120.0;
        prof->deadzone = dz;
        mgr.m_deadzone = dz;
    }

    Float sz = 24.0;
    if (GetFloat(IDC_GLOBAL_ICON_SIZE, sz))
    {
        if (sz < 12.0) sz = 12.0;
        if (sz > 64.0) sz = 64.0;
        prof->iconSize = sz;
        mgr.m_iconSize = sz;
    }

    Float op = 1.0;
    if (GetFloat(IDC_GLOBAL_OPACITY, op))
    {
        if (op < 0.1) op = 0.1;
        if (op > 1.0) op = 1.0;
        prof->opacity = op;
        mgr.m_opacity = op;
    }

    Int32 dm = 0;
    if (GetInt32(IDC_GLOBAL_DISPLAY_MODE, dm))
    {
        prof->displayMode = (PieDisplayMode)dm;
        mgr.m_displayMode = (PieDisplayMode)dm;
    }

    RefreshProfileComboList();
    m_userArea.Redraw();
    UpdateInspector();
}

void PieMenuEditorDialog::SelectProfile(Int32 index)
{
    PieProfileManager& mgr = PieProfileManager::Get();
    if (mgr.GetProfileCount() <= 0)
    {
        m_userArea.SetActiveProfileIndex(-1);
        SetInt32(IDC_PROFILE_COMBO, NOTOK);
        UpdateMenuSettingsUI();
        UpdateHotkeyUI();
        RefreshSubmenuList();
        UpdateInspector();
        m_userArea.Redraw();
        return;
    }

    if (index < 0 || index >= mgr.GetProfileCount())
        index = 0;

    m_userArea.SetActiveProfileIndex(index);
    SetInt32(IDC_PROFILE_COMBO, index);
    UpdateMenuSettingsUI();
    UpdateHotkeyUI();
    RefreshSubmenuList();
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
    {
        SetBool(IDC_HOTKEY_CTRL, false);
        SetBool(IDC_HOTKEY_SHIFT, false);
        SetBool(IDC_HOTKEY_ALT, false);
        SetInt32(IDC_HOTKEY_KEY_COMBO, 0);
        SetString(IDC_HOTKEY_PREVIEW_TEXT, "Assigned: [None]"_s);
        return;
    }

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
        s_activeDialog->IsActive(IDC_INSPECTOR_CMD_NAME) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_CMD_ID) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_SHIFT_NAME) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_SHIFT_ID) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_CTRL_NAME) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_CTRL_ID) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_ALT_NAME) ||
        s_activeDialog->IsActive(IDC_INSPECTOR_ALT_ID) ||
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
    {
        SetString(IDC_GROUP_INSPECTOR, "No Menu Selected"_s);
        SetBool(IDC_INSPECTOR_ENABLED, false);
        Enable(IDC_INSPECTOR_ENABLED, false);
        Enable(IDC_INSPECTOR_COLOR_FIELD, false);
        SetInt32(IDC_INSPECTOR_SUBMENU_COMBO, 0);
        Enable(IDC_INSPECTOR_SUBMENU_COMBO, false);
        Enable(IDC_INSPECTOR_GOTO_SUBMENU, false);
        SetInt32(IDC_INSPECTOR_CMD_ID, 0);
        Enable(IDC_INSPECTOR_CMD_ID, false);
        SetString(IDC_INSPECTOR_CMD_NAME, ""_s);
        Enable(IDC_INSPECTOR_CMD_NAME, false);
        SetInt32(IDC_INSPECTOR_PRESET_COMBO, 0);
        Enable(IDC_INSPECTOR_PRESET_COMBO, false);
        Enable(IDC_INSPECTOR_PRESET_CAPTURE_BTN, false);
        Enable(IDC_INSPECTOR_PRESET_CLEAR_BTN, false);
        SetString(IDC_INSPECTOR_PRESET_STATUS_TEXT, ""_s);

        SetInt32(IDC_INSPECTOR_SHIFT_PRESET_COMBO, 0);
        Enable(IDC_INSPECTOR_SHIFT_PRESET_COMBO, false);
        Enable(IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN, false);
        Enable(IDC_INSPECTOR_SHIFT_PRESET_CLEAR_BTN, false);
        SetString(IDC_INSPECTOR_SHIFT_PRESET_STATUS_TEXT, ""_s);

        SetInt32(IDC_INSPECTOR_CTRL_PRESET_COMBO, 0);
        Enable(IDC_INSPECTOR_CTRL_PRESET_COMBO, false);
        Enable(IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN, false);
        Enable(IDC_INSPECTOR_CTRL_PRESET_CLEAR_BTN, false);
        SetString(IDC_INSPECTOR_CTRL_PRESET_STATUS_TEXT, ""_s);

        SetInt32(IDC_INSPECTOR_ALT_PRESET_COMBO, 0);
        Enable(IDC_INSPECTOR_ALT_PRESET_COMBO, false);
        Enable(IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN, false);
        Enable(IDC_INSPECTOR_ALT_PRESET_CLEAR_BTN, false);
        SetString(IDC_INSPECTOR_ALT_PRESET_STATUS_TEXT, ""_s);
        m_mainIconArea.m_cmdId = NOTOK;
        m_mainIconArea.m_enabled = false;
        m_mainIconArea.Redraw();

        m_shiftIconArea.m_cmdId = NOTOK;
        m_shiftIconArea.m_enabled = false;
        m_shiftIconArea.Redraw();

        m_ctrlIconArea.m_cmdId = NOTOK;
        m_ctrlIconArea.m_enabled = false;
        m_ctrlIconArea.Redraw();

        m_altIconArea.m_cmdId = NOTOK;
        m_altIconArea.m_enabled = false;
        m_altIconArea.Redraw();

        SetInt32(IDC_INSPECTOR_SHIFT_ID, 0);
        SetString(IDC_INSPECTOR_SHIFT_NAME, ""_s);
        Enable(IDC_INSPECTOR_SHIFT_ID, false);
        Enable(IDC_INSPECTOR_SHIFT_NAME, false);
        Enable(IDC_INSPECTOR_SHIFT_SET, false);
        Enable(IDC_INSPECTOR_SHIFT_CLEAR, false);

        SetInt32(IDC_INSPECTOR_CTRL_ID, 0);
        SetString(IDC_INSPECTOR_CTRL_NAME, ""_s);
        Enable(IDC_INSPECTOR_CTRL_ID, false);
        Enable(IDC_INSPECTOR_CTRL_NAME, false);
        Enable(IDC_INSPECTOR_CTRL_SET, false);
        Enable(IDC_INSPECTOR_CTRL_CLEAR, false);

        SetInt32(IDC_INSPECTOR_ALT_ID, 0);
        SetString(IDC_INSPECTOR_ALT_NAME, ""_s);
        Enable(IDC_INSPECTOR_ALT_ID, false);
        Enable(IDC_INSPECTOR_ALT_NAME, false);
        Enable(IDC_INSPECTOR_ALT_SET, false);
        Enable(IDC_INSPECTOR_ALT_CLEAR, false);
        Enable(IDC_INSPECTOR_CLEAR_SLOT, false);
        return;
    }

    SetInt32(IDC_SLOTCOUNT_NUMBER, prof->slotCount, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
    SetInt32(IDC_SLOTCOUNT_SLIDER, prof->slotCount, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);

    Int32 sel = m_userArea.GetSelectedSlot();
    if (sel != CENTER_SLOT_INDEX && (sel < 0 || sel >= prof->slotCount))
        sel = 0;

    String slotName = GetSlotDisplayName(prof->slotCount, sel);
    SetString(IDC_GROUP_INSPECTOR, FormatString("Selected Slot: @"_s, slotName));

    const PieSlot& slot = prof->slots[sel];
    SetBool(IDC_INSPECTOR_ENABLED, slot.enabled);
    Enable(IDC_INSPECTOR_ENABLED, true);

    SetColorField(IDC_INSPECTOR_COLOR_FIELD, slot.accentColor, 1.0, 1.0, 0);
    Enable(IDC_INSPECTOR_COLOR_FIELD, slot.enabled);

    // Update Submenu combo selection
    Int32 subComboSel = 0;
    if (slot.IsSubMenu())
    {
        for (Int32 i = 0; i < (Int32)m_allSubmenus.GetCount(); ++i)
        {
            if (m_allSubmenus[i].name == slot.subMenuName)
            {
                subComboSel = i + 1;
                break;
            }
        }
    }
    SetInt32(IDC_INSPECTOR_SUBMENU_COMBO, subComboSel);
    Enable(IDC_INSPECTOR_SUBMENU_COMBO, slot.enabled);
    Enable(IDC_INSPECTOR_GOTO_SUBMENU, slot.enabled && slot.IsSubMenu());

    m_mainIconArea.m_cmdId = slot.commandId;
    m_mainIconArea.m_enabled = slot.enabled && !slot.IsSubMenu();
    m_mainIconArea.Redraw();

    m_shiftIconArea.m_cmdId = slot.shiftCommandId;
    m_shiftIconArea.m_enabled = slot.enabled && !slot.IsSubMenu();
    m_shiftIconArea.Redraw();

    m_ctrlIconArea.m_cmdId = slot.ctrlCommandId;
    m_ctrlIconArea.m_enabled = slot.enabled && !slot.IsSubMenu();
    m_ctrlIconArea.Redraw();

    m_altIconArea.m_cmdId = slot.altCommandId;
    m_altIconArea.m_enabled = slot.enabled && !slot.IsSubMenu();
    m_altIconArea.Redraw();

    SetInt32(IDC_INSPECTOR_CMD_ID, slot.commandId > 0 ? slot.commandId : 0);
    Enable(IDC_INSPECTOR_CMD_ID, slot.enabled && !slot.IsSubMenu());
    SetString(IDC_INSPECTOR_CMD_NAME, slot.customLabel);
    Enable(IDC_INSPECTOR_CMD_NAME, slot.enabled);

    // Shift action
    SetInt32(IDC_INSPECTOR_SHIFT_ID, slot.shiftCommandId > 0 ? slot.shiftCommandId : 0);
    String shiftDisplay = slot.shiftCommandId > 0 ? (slot.shiftCustomLabel.IsPopulated() ? slot.shiftCustomLabel : GetCommandName(slot.shiftCommandId)) : ""_s;
    SetString(IDC_INSPECTOR_SHIFT_NAME, shiftDisplay);
    Enable(IDC_INSPECTOR_SHIFT_ID, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_SHIFT_NAME, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_SHIFT_SET, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_SHIFT_CLEAR, slot.enabled && !slot.IsSubMenu() && slot.shiftCommandId > 0);

    // Ctrl action
    SetInt32(IDC_INSPECTOR_CTRL_ID, slot.ctrlCommandId > 0 ? slot.ctrlCommandId : 0);
    String ctrlDisplay = slot.ctrlCommandId > 0 ? (slot.ctrlCustomLabel.IsPopulated() ? slot.ctrlCustomLabel : GetCommandName(slot.ctrlCommandId)) : ""_s;
    SetString(IDC_INSPECTOR_CTRL_NAME, ctrlDisplay);
    Enable(IDC_INSPECTOR_CTRL_ID, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_CTRL_NAME, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_CTRL_SET, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_CTRL_CLEAR, slot.enabled && !slot.IsSubMenu() && slot.ctrlCommandId > 0);

    // Alt action
    SetInt32(IDC_INSPECTOR_ALT_ID, slot.altCommandId > 0 ? slot.altCommandId : 0);
    String altDisplay = slot.altCommandId > 0 ? (slot.altCustomLabel.IsPopulated() ? slot.altCustomLabel : GetCommandName(slot.altCommandId)) : ""_s;
    SetString(IDC_INSPECTOR_ALT_NAME, altDisplay);
    Enable(IDC_INSPECTOR_ALT_ID, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_ALT_NAME, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_ALT_SET, slot.enabled && !slot.IsSubMenu());
    Enable(IDC_INSPECTOR_ALT_CLEAR, slot.enabled && !slot.IsSubMenu() && slot.altCommandId > 0);

    Bool isSlotValid = slot.enabled && (!slot.IsSubMenu() && slot.commandId > 0);

    // Main Preset
    UpdatePresetRowUI(
        IDC_INSPECTOR_PRESET_COMBO, IDC_INSPECTOR_PRESET_CAPTURE_BTN,
        IDC_INSPECTOR_PRESET_CLEAR_BTN, IDC_INSPECTOR_PRESET_STATUS_TEXT,
        slot.commandId,
        slot.toolPresetMode, slot.toolPresetName, slot.toolPresetAssetId, slot.toolSettings,
        m_currentDiscoveredPresets,
        isSlotValid);

    // Shift Preset
    Int32 shiftTargetCmd = (slot.shiftCommandId > 0) ? slot.shiftCommandId : slot.commandId;
    UpdatePresetRowUI(
        IDC_INSPECTOR_SHIFT_PRESET_COMBO, IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN,
        IDC_INSPECTOR_SHIFT_PRESET_CLEAR_BTN, IDC_INSPECTOR_SHIFT_PRESET_STATUS_TEXT,
        shiftTargetCmd,
        slot.shiftPresetMode, slot.shiftPresetName, slot.shiftPresetAssetId, slot.shiftSettings,
        m_shiftDiscoveredPresets,
        isSlotValid);

    // Ctrl Preset
    Int32 ctrlTargetCmd = (slot.ctrlCommandId > 0) ? slot.ctrlCommandId : slot.commandId;
    UpdatePresetRowUI(
        IDC_INSPECTOR_CTRL_PRESET_COMBO, IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN,
        IDC_INSPECTOR_CTRL_PRESET_CLEAR_BTN, IDC_INSPECTOR_CTRL_PRESET_STATUS_TEXT,
        ctrlTargetCmd,
        slot.ctrlPresetMode, slot.ctrlPresetName, slot.ctrlPresetAssetId, slot.ctrlSettings,
        m_ctrlDiscoveredPresets,
        isSlotValid);

    // Alt Preset
    Int32 altTargetCmd = (slot.altCommandId > 0) ? slot.altCommandId : slot.commandId;
    UpdatePresetRowUI(
        IDC_INSPECTOR_ALT_PRESET_COMBO, IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN,
        IDC_INSPECTOR_ALT_PRESET_CLEAR_BTN, IDC_INSPECTOR_ALT_PRESET_STATUS_TEXT,
        altTargetCmd,
        slot.altPresetMode, slot.altPresetName, slot.altPresetAssetId, slot.altSettings,
        m_altDiscoveredPresets,
        isSlotValid);

    Enable(IDC_INSPECTOR_CLEAR_SLOT, slot.enabled);
}

void PieMenuEditorDialog::ApplyInspectorChanges()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
    if (!prof)
        return;

    Int32 sel = m_userArea.GetSelectedSlot();
    if (sel != CENTER_SLOT_INDEX && (sel < 0 || sel >= prof->slotCount))
        return;

    PieSlot& slot = prof->slots[sel];
    Bool en = true;
    if (GetBool(IDC_INSPECTOR_ENABLED, en))
        slot.enabled = en;

    Int32 subSel = 0;
    if (GetInt32(IDC_INSPECTOR_SUBMENU_COMBO, subSel))
    {
        if (subSel == 0 && slot.IsSubMenu())
        {
            slot.subMenuName = ""_s;
            Enable(IDC_INSPECTOR_CMD_ID, true);
        }
        else if (subSel > 0 && (subSel - 1) < (Int32)m_allSubmenus.GetCount())
        {
            slot.subMenuName = m_allSubmenus[subSel - 1].name;
            slot.commandId = NOTOK;
            slot.ClearToolPreset();
            SetInt32(IDC_INSPECTOR_CMD_ID, 0);
            Enable(IDC_INSPECTOR_CMD_ID, false);
            Enable(IDC_INSPECTOR_GOTO_SUBMENU, true);
            if (!slot.customLabel.IsPopulated() || slot.customLabel == GetCommandName(slot.commandId))
            {
                slot.customLabel = slot.subMenuName;
                SetString(IDC_INSPECTOR_CMD_NAME, slot.customLabel);
            }
        }
    }

    if (!slot.IsSubMenu())
    {
        Int32 newCmdId = 0;
        if (GetInt32(IDC_INSPECTOR_CMD_ID, newCmdId))
        {
            if (newCmdId > 0)
            {
                if (slot.commandId != newCmdId)
                    slot.ClearToolPreset();
                slot.commandId = newCmdId;
                slot.subMenuName = ""_s;
                if (!slot.customLabel.IsPopulated())
                {
                    slot.customLabel = GetCommandName(slot.commandId);
                    SetString(IDC_INSPECTOR_CMD_NAME, slot.customLabel);
                }
            }
        }

        // Shift Action
        Int32 newShiftId = 0;
        if (GetInt32(IDC_INSPECTOR_SHIFT_ID, newShiftId))
        {
            if (newShiftId > 0)
            {
                if (slot.shiftCommandId != newShiftId)
                {
                    slot.ClearShiftPreset();
                    slot.shiftCustomLabel = GetCommandName(newShiftId);
                    SetString(IDC_INSPECTOR_SHIFT_NAME, slot.shiftCustomLabel);
                }
                slot.shiftCommandId = newShiftId;
            }
            else if (newShiftId == 0 && slot.shiftCommandId > 0)
            {
                slot.ClearShiftAction();
            }
        }

        // Ctrl Action
        Int32 newCtrlId = 0;
        if (GetInt32(IDC_INSPECTOR_CTRL_ID, newCtrlId))
        {
            if (newCtrlId > 0)
            {
                if (slot.ctrlCommandId != newCtrlId)
                {
                    slot.ClearCtrlPreset();
                    slot.ctrlCustomLabel = GetCommandName(newCtrlId);
                    SetString(IDC_INSPECTOR_CTRL_NAME, slot.ctrlCustomLabel);
                }
                slot.ctrlCommandId = newCtrlId;
            }
            else if (newCtrlId == 0 && slot.ctrlCommandId > 0)
            {
                slot.ClearCtrlAction();
            }
        }

        // Alt Action
        Int32 newAltId = 0;
        if (GetInt32(IDC_INSPECTOR_ALT_ID, newAltId))
        {
            if (newAltId > 0)
            {
                if (slot.altCommandId != newAltId)
                {
                    slot.ClearAltPreset();
                    slot.altCustomLabel = GetCommandName(newAltId);
                    SetString(IDC_INSPECTOR_ALT_NAME, slot.altCustomLabel);
                }
                slot.altCommandId = newAltId;
            }
            else if (newAltId == 0 && slot.altCommandId > 0)
            {
                slot.ClearAltAction();
            }
        }
    }

    String customName;
    if (GetString(IDC_INSPECTOR_CMD_NAME, customName))
        slot.customLabel = customName;

    String shiftName;
    if (GetString(IDC_INSPECTOR_SHIFT_NAME, shiftName))
        slot.shiftCustomLabel = shiftName;

    String ctrlName;
    if (GetString(IDC_INSPECTOR_CTRL_NAME, ctrlName))
        slot.ctrlCustomLabel = ctrlName;

    String altName;
    if (GetString(IDC_INSPECTOR_ALT_NAME, altName))
        slot.altCustomLabel = altName;

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
                ApplyInspectorChanges();
                ApplyMenuSettingsChanges();
                ApplyHotkeyChanges();
                SelectProfile(p);
            }
            break;
        }

        case IDC_BTN_NEW_PROFILE:
        {
            ApplyInspectorChanges();
            ApplyMenuSettingsChanges();
            ApplyHotkeyChanges();
            Int32 newIdx = mgr.AddProfile();
            RefreshProfileComboList();
            SelectProfile(newIdx);
            break;
        }

        case IDC_BTN_DUPLICATE_PROFILE:
        {
            ApplyInspectorChanges();
            ApplyMenuSettingsChanges();
            ApplyHotkeyChanges();
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
            if (mgr.GetProfileCount() <= 0)
            {
                MessageDialog("No pie menu to delete."_s);
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

        case IDC_SLOTCOUNT_SLIDER:
        case IDC_SLOTCOUNT_NUMBER:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sc = 8;
                if (id == IDC_SLOTCOUNT_SLIDER ? GetInt32(IDC_SLOTCOUNT_SLIDER, sc) : GetInt32(IDC_SLOTCOUNT_NUMBER, sc))
                {
                    if (sc < MIN_SLOT_COUNT) sc = MIN_SLOT_COUNT;
                    if (sc > MAX_SLOT_COUNT) sc = MAX_SLOT_COUNT;
                    SetInt32(IDC_SLOTCOUNT_SLIDER, sc, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
                    SetInt32(IDC_SLOTCOUNT_NUMBER, sc, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
                    if (prof->slotCount != sc)
                    {
                        prof->slotCount = sc;
                        Int32 sel = m_userArea.GetSelectedSlot();
                        if (sel != CENTER_SLOT_INDEX && sel >= sc)
                            m_userArea.SetSelectedSlot(sc - 1);
                        m_userArea.Redraw();
                        UpdateInspector();
                        RefreshSubmenuList();
                    }
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

        case IDC_PROFILE_SHOW_ITEM_NAME:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Bool sin = true;
                if (GetBool(IDC_PROFILE_SHOW_ITEM_NAME, sin))
                {
                    prof->showItemName = sin;
                    m_userArea.Redraw();
                }
            }
            break;
        }

        case IDC_PROFILE_STAY_OPEN_ON_RELEASE:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Bool so = false;
                if (GetBool(IDC_PROFILE_STAY_OPEN_ON_RELEASE, so))
                {
                    prof->stayOpenOnRelease = so;
                    mgr.m_stayOpenOnRelease = so;
                    Enable(IDC_PROFILE_STAY_OPEN_CENTER, so);
                }
            }
            break;
        }

        case IDC_PROFILE_STAY_OPEN_CENTER:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Bool soc = false;
                if (GetBool(IDC_PROFILE_STAY_OPEN_CENTER, soc))
                {
                    prof->stayOpenWithCenterCommand = soc;
                    mgr.m_stayOpenWithCenterCommand = soc;
                }
            }
            break;
        }

        case IDC_USERAREA_PIE:
        {
            UpdateInspector();
            break;
        }

        case IDC_SUBMENU_BTN_ASSIGN:
        {
            if (m_selectedSubmenu)
            {
                AssignSubmenuToSelectedSlot(m_selectedSubmenu->name);
            }
            break;
        }

        case IDC_SUBMENU_BTN_NEW:
        {
            Int32 newIdx = mgr.AddProfile();
            RefreshProfileComboList();
            SelectProfile(newIdx);
            break;
        }

        case IDC_INSPECTOR_SUBMENU_COMBO:
        {
            ApplyInspectorChanges();
            UpdateInspector();
            break;
        }

        case IDC_INSPECTOR_GOTO_SUBMENU:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX) && prof->slots[sel].IsSubMenu())
                {
                    for (Int32 i = 0; i < mgr.GetProfileCount(); ++i)
                    {
                        PieProfile* p = mgr.GetProfile(i);
                        if (p && p->name == prof->slots[sel].subMenuName)
                        {
                            ApplyInspectorChanges();
                            ApplyMenuSettingsChanges();
                            ApplyHotkeyChanges();
                            SelectProfile(i);
                            break;
                        }
                    }
                }
            }
            break;
        }

        case IDC_INSPECTOR_ENABLED:
        case IDC_INSPECTOR_CMD_ID:
        case IDC_INSPECTOR_SHIFT_ID:
        case IDC_INSPECTOR_CTRL_ID:
        case IDC_INSPECTOR_ALT_ID:
        {
            ApplyInspectorChanges();
            UpdateInspector();
            m_userArea.Redraw();
            break;
        }

        case IDC_INSPECTOR_CMD_NAME:
        case IDC_INSPECTOR_SHIFT_NAME:
        case IDC_INSPECTOR_CTRL_NAME:
        case IDC_INSPECTOR_ALT_NAME:
        {
            ApplyInspectorChanges();
            m_userArea.Redraw();
            break;
        }

        case IDC_INSPECTOR_COLOR_FIELD:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
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

        case IDC_INSPECTOR_SHIFT_SET:
        {
            if (m_selectedCommand && m_selectedCommand->id > 0)
            {
                PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                if (prof)
                {
                    Int32 sel = m_userArea.GetSelectedSlot();
                    if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                    {
                        prof->slots[sel].shiftCommandId = m_selectedCommand->id;
                        prof->slots[sel].shiftCustomLabel = m_selectedCommand->name;
                        prof->slots[sel].ClearShiftPreset();
                        UpdateInspector();
                        m_userArea.Redraw();
                    }
                }
            }
            break;
        }

        case IDC_INSPECTOR_SHIFT_CLEAR:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                {
                    prof->slots[sel].ClearShiftAction();
                    UpdateInspector();
                    m_userArea.Redraw();
                }
            }
            break;
        }

        case IDC_INSPECTOR_CTRL_SET:
        {
            if (m_selectedCommand && m_selectedCommand->id > 0)
            {
                PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                if (prof)
                {
                    Int32 sel = m_userArea.GetSelectedSlot();
                    if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                    {
                        prof->slots[sel].ctrlCommandId = m_selectedCommand->id;
                        prof->slots[sel].ctrlCustomLabel = m_selectedCommand->name;
                        prof->slots[sel].ClearCtrlPreset();
                        UpdateInspector();
                        m_userArea.Redraw();
                    }
                }
            }
            break;
        }

        case IDC_INSPECTOR_CTRL_CLEAR:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                {
                    prof->slots[sel].ClearCtrlAction();
                    UpdateInspector();
                    m_userArea.Redraw();
                }
            }
            break;
        }

        case IDC_INSPECTOR_ALT_SET:
        {
            if (m_selectedCommand && m_selectedCommand->id > 0)
            {
                PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                if (prof)
                {
                    Int32 sel = m_userArea.GetSelectedSlot();
                    if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                    {
                        prof->slots[sel].altCommandId = m_selectedCommand->id;
                        prof->slots[sel].altCustomLabel = m_selectedCommand->name;
                        prof->slots[sel].ClearAltPreset();
                        UpdateInspector();
                        m_userArea.Redraw();
                    }
                }
            }
            break;
        }

        case IDC_INSPECTOR_ALT_CLEAR:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                {
                    prof->slots[sel].ClearAltAction();
                    UpdateInspector();
                    m_userArea.Redraw();
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
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                {
                    prof->slots[sel].commandId = NOTOK;
                    prof->slots[sel].subMenuName = ""_s;
                    prof->slots[sel].customLabel = ""_s;
                    prof->slots[sel].shiftCommandId = NOTOK;
                    prof->slots[sel].ctrlCommandId = NOTOK;
                    prof->slots[sel].altCommandId = NOTOK;
                    prof->slots[sel].ClearAllPresets();
                    UpdateInspector();
                    m_userArea.Redraw();
                }
            }
            break;
        }

        case IDC_INSPECTOR_PRESET_CAPTURE_BTN:
        case IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN:
        case IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN:
        case IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                {
                    PieSlot& slot = prof->slots[sel];
                    if (slot.commandId > 0 && !slot.IsSubMenu())
                    {
                        BaseDocument* doc = GetActiveDocument();
                        if (doc)
                        {
                            Int32 targetCmd = slot.commandId;
                            if (id == IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN)
                                targetCmd = (slot.shiftCommandId > 0) ? slot.shiftCommandId : slot.commandId;
                            else if (id == IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN)
                                targetCmd = (slot.ctrlCommandId > 0) ? slot.ctrlCommandId : slot.commandId;
                            else if (id == IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN)
                                targetCmd = (slot.altCommandId > 0) ? slot.altCommandId : slot.commandId;

                            BaseContainer capturedData;
                            String capturedName = "Active Settings"_s;
                            if (CapturePresetSnapshot(doc, targetCmd, capturedData, capturedName))
                            {
                                if (id == IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN)
                                {
                                    slot.shiftPresetMode = ToolPresetMode::Snapshot;
                                    slot.shiftSettings = capturedData;
                                    slot.shiftPresetName = capturedName;
                                    slot.shiftPresetAssetId = ""_s;
                                }
                                else if (id == IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN)
                                {
                                    slot.ctrlPresetMode = ToolPresetMode::Snapshot;
                                    slot.ctrlSettings = capturedData;
                                    slot.ctrlPresetName = capturedName;
                                    slot.ctrlPresetAssetId = ""_s;
                                }
                                else if (id == IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN)
                                {
                                    slot.altPresetMode = ToolPresetMode::Snapshot;
                                    slot.altSettings = capturedData;
                                    slot.altPresetName = capturedName;
                                    slot.altPresetAssetId = ""_s;
                                }
                                else
                                {
                                    slot.toolPresetMode = ToolPresetMode::Snapshot;
                                    slot.toolSettings = capturedData;
                                    slot.toolPresetName = capturedName;
                                    slot.toolPresetAssetId = ""_s;
                                }
                                UpdateInspector();
                                m_userArea.Redraw();
                            }
                        }
                    }
                }
            }
            break;
        }

        case IDC_INSPECTOR_PRESET_CLEAR_BTN:
        case IDC_INSPECTOR_SHIFT_PRESET_CLEAR_BTN:
        case IDC_INSPECTOR_CTRL_PRESET_CLEAR_BTN:
        case IDC_INSPECTOR_ALT_PRESET_CLEAR_BTN:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                {
                    PieSlot& slot = prof->slots[sel];
                    if (id == IDC_INSPECTOR_SHIFT_PRESET_CLEAR_BTN)
                        slot.ClearShiftPreset();
                    else if (id == IDC_INSPECTOR_CTRL_PRESET_CLEAR_BTN)
                        slot.ClearCtrlPreset();
                    else if (id == IDC_INSPECTOR_ALT_PRESET_CLEAR_BTN)
                        slot.ClearAltPreset();
                    else
                        slot.ClearToolPreset();

                    UpdateInspector();
                    m_userArea.Redraw();
                }
            }
            break;
        }

        case IDC_INSPECTOR_PRESET_COMBO:
        case IDC_INSPECTOR_SHIFT_PRESET_COMBO:
        case IDC_INSPECTOR_CTRL_PRESET_COMBO:
        case IDC_INSPECTOR_ALT_PRESET_COMBO:
        {
            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
            if (prof)
            {
                Int32 sel = m_userArea.GetSelectedSlot();
                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                {
                    PieSlot& slot = prof->slots[sel];
                    Int32 comboVal = 0;
                    if (GetInt32(id, comboVal))
                    {
                        if (id == IDC_INSPECTOR_SHIFT_PRESET_COMBO)
                            HandlePresetComboChange(comboVal, slot.shiftPresetMode, slot.shiftPresetName, slot.shiftPresetAssetId, slot.shiftSettings, m_shiftDiscoveredPresets);
                        else if (id == IDC_INSPECTOR_CTRL_PRESET_COMBO)
                            HandlePresetComboChange(comboVal, slot.ctrlPresetMode, slot.ctrlPresetName, slot.ctrlPresetAssetId, slot.ctrlSettings, m_ctrlDiscoveredPresets);
                        else if (id == IDC_INSPECTOR_ALT_PRESET_COMBO)
                            HandlePresetComboChange(comboVal, slot.altPresetMode, slot.altPresetName, slot.altPresetAssetId, slot.altSettings, m_altDiscoveredPresets);
                        else
                            HandlePresetComboChange(comboVal, slot.toolPresetMode, slot.toolPresetName, slot.toolPresetAssetId, slot.toolSettings, m_currentDiscoveredPresets);

                        UpdateInspector();
                        m_userArea.Redraw();
                    }
                }
            }
            break;
        }


        case IDC_GLOBAL_RADIUS:
        {
            Float outR = 112.0;
            if (GetFloat(IDC_GLOBAL_RADIUS, outR))
            {
                if (outR < 30.0) outR = 30.0;
                if (outR > 300.0) outR = 300.0;
                Int32 curIdx = m_userArea.GetActiveProfileIndex();
                PieProfile* prof = mgr.GetProfile(curIdx);
                if (prof)
                {
                    prof->outerRadius = outR;
                    prof->radius = outR;
                }
                mgr.m_outerRadius = outR;
                mgr.m_radius = outR;
                m_userArea.Redraw();
            }
            break;
        }

        case IDC_GLOBAL_INNER_RADIUS:
        {
            Float inR = 36.0;
            if (GetFloat(IDC_GLOBAL_INNER_RADIUS, inR))
            {
                if (inR < 10.0) inR = 10.0;
                if (inR > 250.0) inR = 250.0;
                Int32 curIdx = m_userArea.GetActiveProfileIndex();
                PieProfile* prof = mgr.GetProfile(curIdx);
                if (prof)
                    prof->innerRadius = inR;
                mgr.m_innerRadius = inR;
                m_userArea.Redraw();
            }
            break;
        }

        case IDC_GLOBAL_DEADZONE:
        {
            Float dz = 26.0;
            if (GetFloat(IDC_GLOBAL_DEADZONE, dz))
            {
                if (dz < 5.0) dz = 5.0;
                if (dz > 120.0) dz = 120.0;
                Int32 curIdx = m_userArea.GetActiveProfileIndex();
                PieProfile* prof = mgr.GetProfile(curIdx);
                if (prof)
                    prof->deadzone = dz;
                mgr.m_deadzone = dz;
                m_userArea.Redraw();
            }
            break;
        }

        case IDC_GLOBAL_ICON_SIZE:
        {
            Float sz = 24.0;
            if (GetFloat(IDC_GLOBAL_ICON_SIZE, sz))
            {
                if (sz < 12.0) sz = 12.0;
                if (sz > 64.0) sz = 64.0;
                Int32 curIdx = m_userArea.GetActiveProfileIndex();
                PieProfile* prof = mgr.GetProfile(curIdx);
                if (prof)
                    prof->iconSize = sz;
                mgr.m_iconSize = sz;
                m_userArea.Redraw();
            }
            break;
        }

        case IDC_GLOBAL_OPACITY:
        {
            Float op = 1.0;
            if (GetFloat(IDC_GLOBAL_OPACITY, op))
            {
                if (op < 0.1) op = 0.1;
                if (op > 1.0) op = 1.0;
                Int32 curIdx = m_userArea.GetActiveProfileIndex();
                PieProfile* prof = mgr.GetProfile(curIdx);
                if (prof)
                    prof->opacity = op;
                mgr.m_opacity = op;
                m_userArea.Redraw();
            }
            break;
        }

        case IDC_GLOBAL_DISPLAY_MODE:
        {
            Int32 mode = 0;
            if (GetInt32(IDC_GLOBAL_DISPLAY_MODE, mode))
            {
                Int32 curIdx = m_userArea.GetActiveProfileIndex();
                PieProfile* prof = mgr.GetProfile(curIdx);
                if (prof)
                    prof->displayMode = (PieDisplayMode)mode;
                mgr.m_displayMode = (PieDisplayMode)mode;
                m_userArea.Redraw();
            }
            break;
        }

        case IDC_BTN_SAVE:
        {
            ApplyInspectorChanges();
            ApplyMenuSettingsChanges();
            ApplyHotkeyChanges();

            if (mgr.SaveToFile())
                GePrint("PieMenu: Successfully saved configuration to preferences."_s);
            break;
        }

        case IDC_BTN_EXPORT:
        {
            ApplyInspectorChanges();
            ApplyMenuSettingsChanges();
            ApplyHotkeyChanges();

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

                SetFloat(IDC_GLOBAL_RADIUS, mgr.m_outerRadius);
                SetFloat(IDC_GLOBAL_INNER_RADIUS, mgr.m_innerRadius);
                SetFloat(IDC_GLOBAL_DEADZONE, mgr.m_deadzone);
                SetFloat(IDC_GLOBAL_ICON_SIZE, mgr.m_iconSize);
                SetPercent(IDC_GLOBAL_OPACITY, mgr.m_opacity, 10.0, 100.0, 1.0);
                SetInt32(IDC_GLOBAL_DISPLAY_MODE, (Int32)mgr.m_displayMode);

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
        case BFM_DRAGRECEIVE:
        {
            Int32 type = 0;
            void* object = nullptr;
            GetDragObject(msg, &type, &object);

            if (type == DRAGTYPE_COMMAND)
            {
                Int32 cmdId = PieMenuEditorArea::ExtractCommandId(type, object, msg);
                if (cmdId > 0)
                {
                    PieProfileManager& mgr = PieProfileManager::Get();
                    PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                    if (prof)
                    {
                        Int32 sel = m_userArea.GetSelectedSlot();
                        if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                        {
                            PieSlot& slot = prof->slots[sel];

                            Bool inShift = CheckDropArea(IDC_GROUP_SHIFT_ACTION, msg, true, true) ||
                                           CheckDropArea(IDC_INSPECTOR_SHIFT_ICON, msg, true, true) ||
                                           CheckDropArea(IDC_INSPECTOR_SHIFT_NAME, msg, true, true) ||
                                           CheckDropArea(IDC_INSPECTOR_SHIFT_ID, msg, true, true);

                            Bool inCtrl = CheckDropArea(IDC_GROUP_CTRL_ACTION, msg, true, true) ||
                                          CheckDropArea(IDC_INSPECTOR_CTRL_ICON, msg, true, true) ||
                                          CheckDropArea(IDC_INSPECTOR_CTRL_NAME, msg, true, true) ||
                                          CheckDropArea(IDC_INSPECTOR_CTRL_ID, msg, true, true);

                            Bool inAlt = CheckDropArea(IDC_GROUP_ALT_ACTION, msg, true, true) ||
                                         CheckDropArea(IDC_INSPECTOR_ALT_ICON, msg, true, true) ||
                                         CheckDropArea(IDC_INSPECTOR_ALT_NAME, msg, true, true) ||
                                         CheckDropArea(IDC_INSPECTOR_ALT_ID, msg, true, true);

                            Bool inMain = CheckDropArea(IDC_GROUP_MAIN_ACTION, msg, true, true) ||
                                          CheckDropArea(IDC_INSPECTOR_MAIN_ICON, msg, true, true) ||
                                          CheckDropArea(IDC_INSPECTOR_CMD_NAME, msg, true, true) ||
                                          CheckDropArea(IDC_INSPECTOR_CMD_ID, msg, true, true);

                            Bool hoverShift = inShift;
                            Bool hoverCtrl = inCtrl;
                            Bool hoverAlt = inAlt;
                            Bool hoverMain = inMain;

                            if (msg.GetInt32(BFM_DRAG_FINISHED))
                            {
                                hoverShift = false;
                                hoverCtrl = false;
                                hoverAlt = false;
                                hoverMain = false;
                            }

                            if (m_mainIconArea.m_dragHover != hoverMain ||
                                m_shiftIconArea.m_dragHover != hoverShift ||
                                m_ctrlIconArea.m_dragHover != hoverCtrl ||
                                m_altIconArea.m_dragHover != hoverAlt)
                            {
                                m_mainIconArea.m_dragHover = hoverMain;
                                m_shiftIconArea.m_dragHover = hoverShift;
                                m_ctrlIconArea.m_dragHover = hoverCtrl;
                                m_altIconArea.m_dragHover = hoverAlt;
                                m_mainIconArea.Redraw();
                                m_shiftIconArea.Redraw();
                                m_ctrlIconArea.Redraw();
                                m_altIconArea.Redraw();
                            }

                            if (inShift || inCtrl || inAlt || inMain)
                            {
                                if (msg.GetInt32(BFM_DRAG_FINISHED))
                                {
                                    slot.enabled = true;
                                    String cmdName = GetCommandName(cmdId);

                                    if (inShift)
                                    {
                                        slot.shiftCommandId = cmdId;
                                        slot.shiftCustomLabel = cmdName;
                                        slot.ClearShiftPreset();
                                    }
                                    else if (inCtrl)
                                    {
                                        slot.ctrlCommandId = cmdId;
                                        slot.ctrlCustomLabel = cmdName;
                                        slot.ClearCtrlPreset();
                                    }
                                    else if (inAlt)
                                    {
                                        slot.altCommandId = cmdId;
                                        slot.altCustomLabel = cmdName;
                                        slot.ClearAltPreset();
                                    }
                                    else if (inMain)
                                    {
                                        slot.commandId = cmdId;
                                        slot.customLabel = cmdName;
                                        slot.subMenuName = ""_s;
                                        slot.ClearToolPreset();
                                    }

                                    UpdateInspector();
                                    m_userArea.Redraw();
                                }
                                return SetDragDestination(MOUSE_POINT_HAND);
                            }
                        }
                    }
                }
            }
            else if (type == DRAGTYPE_ASSET)
            {
                if (msg.GetInt32(BFM_DRAG_FINISHED))
                {
                    const maxon::DragAndDropDataAssetArray* dndData = static_cast<const maxon::DragAndDropDataAssetArray*>(object);
                    if (dndData && dndData->GetAssetDescriptions().GetCount() > 0)
                    {
                        const maxon::DndAsset& dndAsset = dndData->GetAssetDescriptions()[0];
                        const maxon::AssetDescription& assetDesc = dndAsset.Get<0>();
                        if (assetDesc)
                        {
                            PieProfileManager& mgr = PieProfileManager::Get();
                            PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                            if (prof)
                            {
                                Int32 sel = m_userArea.GetSelectedSlot();
                                if (sel >= 0 && (sel < prof->slotCount || sel == CENTER_SLOT_INDEX))
                                {
                                    PieSlot& slot = prof->slots[sel];
                                    maxon::String assetIdStr = assetDesc.GetId().ToString();
                                    maxon::String assetName = assetDesc.GetMetaString(maxon::OBJECT::BASE::NAME) iferr_ignore("GetMetaString");
                                    if (assetName.IsEmpty())
                                        assetName = assetIdStr;

                                    Bool inShiftPreset = CheckDropArea(IDC_INSPECTOR_SHIFT_PRESET_COMBO, msg, true, true) ||
                                                         CheckDropArea(IDC_INSPECTOR_SHIFT_PRESET_CAPTURE_BTN, msg, true, true) ||
                                                         CheckDropArea(IDC_INSPECTOR_SHIFT_PRESET_CLEAR_BTN, msg, true, true) ||
                                                         CheckDropArea(IDC_INSPECTOR_SHIFT_PRESET_STATUS_TEXT, msg, true, true);
                                    Bool inCtrlPreset = CheckDropArea(IDC_INSPECTOR_CTRL_PRESET_COMBO, msg, true, true) ||
                                                        CheckDropArea(IDC_INSPECTOR_CTRL_PRESET_CAPTURE_BTN, msg, true, true) ||
                                                        CheckDropArea(IDC_INSPECTOR_CTRL_PRESET_CLEAR_BTN, msg, true, true) ||
                                                        CheckDropArea(IDC_INSPECTOR_CTRL_PRESET_STATUS_TEXT, msg, true, true);
                                    Bool inAltPreset = CheckDropArea(IDC_INSPECTOR_ALT_PRESET_COMBO, msg, true, true) ||
                                                       CheckDropArea(IDC_INSPECTOR_ALT_PRESET_CAPTURE_BTN, msg, true, true) ||
                                                       CheckDropArea(IDC_INSPECTOR_ALT_PRESET_CLEAR_BTN, msg, true, true) ||
                                                       CheckDropArea(IDC_INSPECTOR_ALT_PRESET_STATUS_TEXT, msg, true, true);

                                    if (inShiftPreset)
                                    {
                                        slot.shiftPresetMode = ToolPresetMode::Asset;
                                        slot.shiftPresetAssetId = assetIdStr;
                                        slot.shiftPresetName = assetName;
                                    }
                                    else if (inCtrlPreset)
                                    {
                                        slot.ctrlPresetMode = ToolPresetMode::Asset;
                                        slot.ctrlPresetAssetId = assetIdStr;
                                        slot.ctrlPresetName = assetName;
                                    }
                                    else if (inAltPreset)
                                    {
                                        slot.altPresetMode = ToolPresetMode::Asset;
                                        slot.altPresetAssetId = assetIdStr;
                                        slot.altPresetName = assetName;
                                    }
                                    else
                                    {
                                        slot.toolPresetMode = ToolPresetMode::Asset;
                                        slot.toolPresetAssetId = assetIdStr;
                                        slot.toolPresetName = assetName;
                                    }
                                    slot.enabled = true;

                                    UpdateInspector();
                                    m_userArea.Redraw();
                                }
                            }
                        }
                    }
                }
                return SetDragDestination(MOUSE_POINT_HAND);
            }
            break;
        }

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
            else if (msg.GetInt32(BFM_ACTION_ID) == IDC_SLOTCOUNT_SLIDER || msg.GetInt32(BFM_ACTION_ID) == IDC_SLOTCOUNT_NUMBER)
            {
                PieProfileManager& mgr = PieProfileManager::Get();
                PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                if (prof)
                {
                    Int32 actionId = msg.GetInt32(BFM_ACTION_ID);
                    Int32 sc = 8;
                    if (actionId == IDC_SLOTCOUNT_SLIDER ? GetInt32(IDC_SLOTCOUNT_SLIDER, sc) : GetInt32(IDC_SLOTCOUNT_NUMBER, sc))
                    {
                        if (sc < MIN_SLOT_COUNT) sc = MIN_SLOT_COUNT;
                        if (sc > MAX_SLOT_COUNT) sc = MAX_SLOT_COUNT;
                        SetInt32(IDC_SLOTCOUNT_SLIDER, sc, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
                        SetInt32(IDC_SLOTCOUNT_NUMBER, sc, MIN_SLOT_COUNT, MAX_SLOT_COUNT, 1);
                        if (prof->slotCount != sc)
                        {
                            prof->slotCount = sc;
                            Int32 sel = m_userArea.GetSelectedSlot();
                            if (sel != CENTER_SLOT_INDEX && sel >= sc)
                                m_userArea.SetSelectedSlot(sc - 1);
                            m_userArea.Redraw();
                            UpdateInspector();
                            RefreshSubmenuList();
                        }
                    }
                }
            }
            else if (msg.GetInt32(BFM_ACTION_ID) == IDC_GLOBAL_RADIUS ||
                     msg.GetInt32(BFM_ACTION_ID) == IDC_GLOBAL_INNER_RADIUS ||
                     msg.GetInt32(BFM_ACTION_ID) == IDC_GLOBAL_DEADZONE ||
                     msg.GetInt32(BFM_ACTION_ID) == IDC_GLOBAL_ICON_SIZE ||
                     msg.GetInt32(BFM_ACTION_ID) == IDC_GLOBAL_OPACITY ||
                     msg.GetInt32(BFM_ACTION_ID) == IDC_GLOBAL_DISPLAY_MODE)
            {
                PieProfileManager& mgr = PieProfileManager::Get();
                PieProfile* prof = mgr.GetProfile(m_userArea.GetActiveProfileIndex());
                if (prof)
                {
                    Float outR = 112.0;
                    if (GetFloat(IDC_GLOBAL_RADIUS, outR))
                    {
                        if (outR < 30.0) outR = 30.0;
                        if (outR > 300.0) outR = 300.0;
                        prof->outerRadius = outR;
                        prof->radius = outR;
                        mgr.m_outerRadius = outR;
                        mgr.m_radius = outR;
                    }
                    Float inR = 36.0;
                    if (GetFloat(IDC_GLOBAL_INNER_RADIUS, inR))
                    {
                        if (inR < 10.0) inR = 10.0;
                        if (inR > 250.0) inR = 250.0;
                        prof->innerRadius = inR;
                        mgr.m_innerRadius = inR;
                    }
                    Float dz = 26.0;
                    if (GetFloat(IDC_GLOBAL_DEADZONE, dz))
                    {
                        if (dz < 5.0) dz = 5.0;
                        if (dz > 120.0) dz = 120.0;
                        prof->deadzone = dz;
                        mgr.m_deadzone = dz;
                    }
                    Float sz = 24.0;
                    if (GetFloat(IDC_GLOBAL_ICON_SIZE, sz))
                    {
                        if (sz < 12.0) sz = 12.0;
                        if (sz > 64.0) sz = 64.0;
                        prof->iconSize = sz;
                        mgr.m_iconSize = sz;
                    }
                    Float op = 1.0;
                    if (GetFloat(IDC_GLOBAL_OPACITY, op))
                    {
                        if (op < 0.1) op = 0.1;
                        if (op > 1.0) op = 1.0;
                        prof->opacity = op;
                        mgr.m_opacity = op;
                    }
                    Int32 dm = 0;
                    if (GetInt32(IDC_GLOBAL_DISPLAY_MODE, dm))
                    {
                        prof->displayMode = (PieDisplayMode)dm;
                        mgr.m_displayMode = (PieDisplayMode)dm;
                    }
                    m_userArea.Redraw();
                }
            }
            break;
        }
    }

    return GeDialog::Message(msg, result);
}

void PieMenuEditorDialog::DestroyWindow()
{
    // Apply any pending edits before saving
    ApplyInspectorChanges();
    ApplyMenuSettingsChanges();
    ApplyHotkeyChanges();

    PieProfileManager& mgr = PieProfileManager::Get();

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
        AutoBitmap("piemenu.png"_s),
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
    return m_dialog.Open(DLG_TYPE::ASYNC, PLUGIN_ID_PIEMENU_EDITOR_COMMAND, -1, -1, 880, 720);
}

Bool PieMenuEditorCommand::RestoreLayout(void* secret)
{
    return m_dialog.RestoreLayout(PLUGIN_ID_PIEMENU_EDITOR_COMMAND, 0, secret);
}

} // namespace cinema
