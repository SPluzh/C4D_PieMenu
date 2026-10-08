#include "piemenu_editor_dialog.h"
#include "c4d_general.h"

namespace cinema
{

enum
{
    IDC_PROFILE_COMBO = 1000,
    IDC_SLOTCOUNT_COMBO,
    IDC_USERAREA_PIE,
    IDC_GROUP_INSPECTOR,
    IDC_INSPECTOR_SLOT_TITLE,
    IDC_INSPECTOR_ENABLED,
    IDC_INSPECTOR_CMD_ID,
    IDC_INSPECTOR_CMD_NAME,
    IDC_INSPECTOR_COLOR_FIELD,
    IDC_INSPECTOR_CLEAR_SLOT,
    IDC_GLOBAL_CTRLSHIFT,
    IDC_GLOBAL_RADIUS,
    IDC_GLOBAL_DEADZONE,
    IDC_BTN_SAVE,
    IDC_BTN_RESET,
    IDC_BTN_CLOSE
};

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

PieMenuEditorDialog::PieMenuEditorDialog()
{
}

Bool PieMenuEditorDialog::CreateLayout()
{
    SetTitle("Pie Menu Configuration"_s);

    // 1. Header / instructions
    GroupBegin(0, BFH_SCALEFIT, 1, 0, ""_s, 0);
    GroupBorderSpace(12, 12, 12, 6);
    AddStaticText(0, BFH_LEFT, 0, 0, "Cinema 4D Radial Pie Menu - Visual Configuration"_s, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Drag commands from 'Customize Commands' onto any slot, or edit properties below."_s, 0);
    GroupEnd();

    // 2. Profile selection bar
    GroupBegin(0, BFH_SCALEFIT, 4, 0, ""_s, 0);
    GroupBorderSpace(12, 0, 12, 6);
    AddStaticText(0, BFH_LEFT, 0, 0, "Active Profile:"_s, 0);
    AddComboBox(IDC_PROFILE_COMBO, BFH_LEFT, 160, 0);
    AddStaticText(0, BFH_LEFT, 0, 0, "Slots:"_s, 0);
    AddComboBox(IDC_SLOTCOUNT_COMBO, BFH_LEFT, 90, 0);
    GroupEnd();

    // 3. UserArea preview
    GroupBegin(0, BFH_CENTER, 1, 0, ""_s, 0);
    GroupBorderSpace(12, 4, 12, 8);
    AddUserArea(IDC_USERAREA_PIE, BFH_CENTER | BFV_CENTER, 360, 360);
    AttachUserArea(m_userArea, IDC_USERAREA_PIE);
    GroupEnd();

    // 4. Selected Slot Inspector group
    GroupBegin(IDC_GROUP_INSPECTOR, BFH_SCALEFIT, 2, 0, "Selected Slot Properties"_s, BORDER_GROUP_IN);
    GroupBorderSpace(12, 8, 12, 8);

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

    // 5. Global Settings group
    GroupBegin(0, BFH_SCALEFIT, 4, 0, "Global Behavior"_s, BORDER_GROUP_IN);
    GroupBorderSpace(12, 8, 12, 8);

    AddCheckbox(IDC_GLOBAL_CTRLSHIFT, BFH_LEFT, 0, 0, "Hold Ctrl + Shift trigger"_s);
    AddStaticText(0, BFH_LEFT, 0, 0, ""_s, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Menu Radius (px):"_s, 0);
    AddEditNumber(IDC_GLOBAL_RADIUS, BFH_SCALEFIT, 60, 0);

    AddStaticText(0, BFH_LEFT, 0, 0, "Deadzone (px):"_s, 0);
    AddEditNumber(IDC_GLOBAL_DEADZONE, BFH_SCALEFIT, 60, 0);

    GroupEnd();

    // 6. Action buttons
    GroupBegin(0, BFH_SCALEFIT, 3, 0, ""_s, 0);
    GroupBorderSpace(12, 8, 12, 12);
    AddButton(IDC_BTN_SAVE, BFH_SCALEFIT, 0, 0, "Save Configuration"_s);
    AddButton(IDC_BTN_RESET, BFH_SCALEFIT, 0, 0, "Reset to Defaults"_s);
    AddButton(IDC_BTN_CLOSE, BFH_SCALEFIT, 0, 0, "Close"_s);
    GroupEnd();

    return true;
}

Bool PieMenuEditorDialog::InitValues()
{
    PieProfileManager& mgr = PieProfileManager::Get();

    // Populate profiles combo
    FreeChildren(IDC_PROFILE_COMBO);
    for (Int32 i = 0; i < mgr.GetProfileCount(); ++i)
    {
        PieProfile* prof = mgr.GetProfile(i);
        if (prof)
            AddChild(IDC_PROFILE_COMBO, i, prof->name);
    }
    SetInt32(IDC_PROFILE_COMBO, m_userArea.GetActiveProfileIndex());

    // Populate slot counts
    FreeChildren(IDC_SLOTCOUNT_COMBO);
    AddChild(IDC_SLOTCOUNT_COMBO, 0, "4 Slots"_s);
    AddChild(IDC_SLOTCOUNT_COMBO, 1, "8 Slots"_s);

    // Global settings
    SetBool(IDC_GLOBAL_CTRLSHIFT, mgr.m_useCtrlShift);
    SetFloat(IDC_GLOBAL_RADIUS, mgr.m_radius);
    SetFloat(IDC_GLOBAL_DEADZONE, mgr.m_deadzone);

    UpdateInspector();
    return true;
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
        case IDC_PROFILE_COMBO:
        {
            Int32 p = 0;
            if (GetInt32(IDC_PROFILE_COMBO, p))
            {
                m_userArea.SetActiveProfileIndex(p);
                UpdateInspector();
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

        case IDC_GLOBAL_CTRLSHIFT:
        {
            Bool cs = true;
            if (GetBool(IDC_GLOBAL_CTRLSHIFT, cs))
                mgr.m_useCtrlShift = cs;
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
            if (mgr.SaveToFile())
                GePrint("PieMenu: Successfully saved configuration to preferences."_s);
            break;
        }

        case IDC_BTN_RESET:
        {
            mgr.InitDefaults();
            mgr.SaveToFile();
            InitValues();
            m_userArea.Redraw();
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
    return m_dialog.Open(DLG_TYPE::ASYNC, PLUGIN_ID_PIEMENU_EDITOR_COMMAND, -1, -1, 480, 640);
}

Bool PieMenuEditorCommand::RestoreLayout(void* secret)
{
    return m_dialog.RestoreLayout(PLUGIN_ID_PIEMENU_EDITOR_COMMAND, 0, secret);
}

} // namespace cinema
