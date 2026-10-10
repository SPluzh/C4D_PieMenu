#include "piemenu_data.h"
#include <cmath>
#include "c4d_general.h"
#include "c4d_libs/lib_iconcollection.h"
#include "windows_include.h"

namespace cinema
{

static const Int32 PIE_CONFIG_MAGIC_V2 = 0x50494532; // "PIE2"

maxon::String GetContextName(PieContextMode ctx)
{
    switch (ctx)
    {
        case PieContextMode::Global:   return "Global (All Modes)"_s;
        case PieContextMode::Polygons: return "Polygons Mode"_s;
        case PieContextMode::Edges:    return "Edges Mode"_s;
        case PieContextMode::Points:   return "Points Mode"_s;
        case PieContextMode::Objects:  return "Objects / Model Mode"_s;
        case PieContextMode::UV:       return "UV Edit Mode"_s;
        default: return "Global"_s;
    }
}

maxon::String GetContextShortName(PieContextMode ctx)
{
    switch (ctx)
    {
        case PieContextMode::Global:   return "Global"_s;
        case PieContextMode::Polygons: return "Polygons"_s;
        case PieContextMode::Edges:    return "Edges"_s;
        case PieContextMode::Points:   return "Points"_s;
        case PieContextMode::Objects:  return "Objects"_s;
        case PieContextMode::UV:       return "UV"_s;
        default: return "Global"_s;
    }
}

PieContextMode GetContextModeForDoc(BaseDocument* doc)
{
    if (!doc)
        return PieContextMode::Global;

    Int32 mode = doc->GetMode();
    if (mode == Mpolygons)
        return PieContextMode::Polygons;
    if (mode == Medges)
        return PieContextMode::Edges;
    if (mode == Mpoints)
        return PieContextMode::Points;
    if (mode == Mobject || mode == Mmodel)
        return PieContextMode::Objects;
    if (mode == Muvpoints || mode == Muvpolygons || mode == Muvedges)
        return PieContextMode::UV;

    return PieContextMode::Global;
}

static maxon::String GetKeyNameString(Int32 vkCode)
{
    if (vkCode >= 'A' && vkCode <= 'Z')
    {
        Char c = (Char)vkCode;
        return maxon::String(&c, 1);
    }
    if (vkCode >= 'a' && vkCode <= 'z')
    {
        Char c = (Char)toupper(vkCode);
        return maxon::String(&c, 1);
    }
    if (vkCode >= '0' && vkCode <= '9')
    {
        Char c = (Char)vkCode;
        return maxon::String(&c, 1);
    }
#if defined(MAXON_TARGET_WINDOWS)
    if (vkCode >= VK_F1 && vkCode <= VK_F12)
    {
        return FormatString("F@"_s, (vkCode - VK_F1 + 1));
    }
    switch (vkCode)
    {
        case VK_TAB: return "Tab"_s;
        case VK_SPACE: return "Space"_s;
        case VK_OEM_3: return "~ (Tilde)"_s;
        case VK_OEM_4: return "["_s;
        case VK_OEM_6: return "]"_s;
        case VK_OEM_COMMA: return ","_s;
        case VK_OEM_PERIOD: return "."_s;
        case VK_OEM_1: return ";"_s;
        case VK_OEM_7: return "'"_s;
        case VK_OEM_2: return "/"_s;
        case VK_OEM_MINUS: return "-"_s;
        case VK_OEM_PLUS: return "="_s;
        default: break;
    }
#endif
    if (vkCode > 0)
        return FormatString("Key #@"_s, vkCode);

    return maxon::String();
}

maxon::String PieHotkey::GetDisplayString() const
{
    if (!enabled)
        return "Disabled"_s;

    maxon::BaseArray<maxon::String> parts;
    if (ctrl)
        parts.Append("Ctrl"_s) iferr_ignore("Append");
    if (alt)
        parts.Append("Alt"_s) iferr_ignore("Append");
    if (shift)
        parts.Append("Shift"_s) iferr_ignore("Append");

    if (vkCode != 0)
    {
        maxon::String kn = GetKeyNameString(vkCode);
        if (kn.IsPopulated())
            parts.Append(kn) iferr_ignore("Append");
    }

    if (parts.IsEmpty())
        return "None"_s;

    maxon::String res;
    for (Int32 i = 0; i < (Int32)parts.GetCount(); ++i)
    {
        if (i > 0)
            res += " + "_s;
        res += parts[i];
    }

    return res;
}

Bool PieHotkey::IsPhysicallyDown() const
{
    if (!enabled)
        return false;

#if defined(MAXON_TARGET_WINDOWS)
    bool cDown = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
    bool sDown = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
    bool aDown = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);

    if (ctrl != cDown)
        return false;
    if (shift != sDown)
        return false;
    if (alt != aDown)
        return false;

    if (vkCode == 0)
    {
        // Modifiers-only combo requires at least one modifier
        return (ctrl || shift || alt);
    }

    Int32 vk = vkCode;
    if (vk >= 'a' && vk <= 'z')
        vk = toupper(vk);

    return (GetAsyncKeyState(vk) & 0x8000) != 0;
#else
    return false;
#endif
}

Bool PieHotkey::IsMatch(Bool ctrlDown, Bool shiftDown, Bool altDown, Int32 key) const
{
    if (!enabled)
        return false;

    if (ctrl != ctrlDown)
        return false;
    if (shift != shiftDown)
        return false;
    if (alt != altDown)
        return false;

    if (vkCode == 0)
    {
        return (ctrl || shift || alt);
    }

    Int32 vk = vkCode;
    if (vk >= 'a' && vk <= 'z')
        vk = toupper(vk);

    Int32 k = key;
    if (k >= 'a' && k <= 'z')
        k = toupper(k);

    return (vk == k);
}

maxon::String PieSlot::GetEffectiveTitle() const
{
    if (customLabel.IsPopulated())
        return customLabel;

    if (commandId > 0)
    {
        String name = GetCommandName(commandId);
        if (name.IsPopulated())
            return name;
        return FormatString("Cmd @"_s, commandId);
    }

    return "Empty"_s;
}

Bool PieSlot::GetIconBitmap(IconData& outIcon) const
{
    if (commandId > 0)
    {
        return GetIcon(commandId, &outIcon);
    }
    return false;
}

Float PieProfile::GetSlotAngle(Int32 index) const
{
    if (slotCount == 4)
    {
        switch (index)
        {
            case 0: return -PI * 0.5; // Up
            case 1: return 0.0;       // Right
            case 2: return PI * 0.5;  // Down
            case 3: return PI;        // Left
            default: return 0.0;
        }
    }
    else
    {
        // 8-way: 0=Up, 1=UpRight, 2=Right, 3=DownRight, 4=Down, 5=DownLeft, 6=Left, 7=UpLeft
        switch (index)
        {
            case 0: return -PI * 0.5;           // Up (-90)
            case 1: return -PI * 0.25;          // Up-Right (-45)
            case 2: return 0.0;                 // Right (0)
            case 3: return PI * 0.25;           // Down-Right (+45)
            case 4: return PI * 0.5;            // Down (+90)
            case 5: return PI * 0.75;           // Down-Left (+135)
            case 6: return PI;                  // Left (180)
            case 7: return -PI * 0.75;          // Up-Left (-135)
            default: return 0.0;
        }
    }
}

Int32 PieProfile::AngleToSlot(Float deg, Float dist, Float deadzone) const
{
    if (dist < deadzone)
        return NOTOK;

    if (slotCount == 4)
    {
        if (deg >= -135.0 && deg < -45.0) return 0; // Up
        if (deg >= -45.0 && deg < 45.0)   return 1; // Right
        if (deg >= 45.0 && deg < 135.0)   return 2; // Down
        return 3;                                   // Left
    }
    else
    {
        if (deg >= -112.5 && deg < -67.5) return 0; // Up
        if (deg >= -67.5  && deg < -22.5) return 1; // Up-Right
        if (deg >= -22.5  && deg < 22.5)  return 2; // Right
        if (deg >= 22.5   && deg < 67.5)  return 3; // Down-Right
        if (deg >= 67.5   && deg < 112.5) return 4; // Down
        if (deg >= 112.5  && deg < 157.5) return 5; // Down-Left
        if (deg >= 157.5  || deg < -157.5) return 6; // Left
        return 7;                                   // Up-Left
    }
}

PieProfileManager& PieProfileManager::Get()
{
    static PieProfileManager instance;
    return instance;
}

PieProfileManager::PieProfileManager()
{
    InitDefaults();
    LoadFromFile();
}

void PieProfileManager::InitDefaults()
{
    m_profiles.Reset();

    // -------------------------------------------------------------
    // PROFILE 0: POLYGONS MODE
    // -------------------------------------------------------------
    {
        PieProfile p;
        p.name = "Polygons"_s;
        p.context = PieContextMode::Polygons;
        p.slotCount = 8;
        p.hotkey.enabled = true;
        p.hotkey.ctrl = true;
        p.hotkey.shift = true;
        p.hotkey.alt = false;
        p.hotkey.vkCode = 0;

        // 0: Up -> Extrude
        p.slots[0].commandId = 1011152;
        p.slots[0].customLabel = "Extrude"_s;
        p.slots[0].accentColor = Vector(0.15, 0.85, 0.45); // Mint Green

        // 1: Up-Right -> Inset / Extrude Inner
        p.slots[1].commandId = 1011153;
        p.slots[1].customLabel = "Inner Extrude"_s;
        p.slots[1].accentColor = Vector(0.2, 0.8, 0.6);

        // 2: Right -> Bevel
        p.slots[2].commandId = 1011155;
        p.slots[2].customLabel = "Bevel"_s;
        p.slots[2].accentColor = Vector(0.25, 0.65, 1.0); // Blue

        // 3: Down-Right -> Bridge
        p.slots[3].commandId = 431000010;
        p.slots[3].customLabel = "Bridge"_s;
        p.slots[3].accentColor = Vector(0.5, 0.5, 1.0);

        // 4: Down -> Loop Cut
        p.slots[4].commandId = 1038596;
        p.slots[4].customLabel = "Loop Cut"_s;
        p.slots[4].accentColor = Vector(1.0, 0.75, 0.2); // Amber

        // 5: Down-Left -> Subdivide
        p.slots[5].commandId = 1011170;
        p.slots[5].customLabel = "Subdivide"_s;
        p.slots[5].accentColor = Vector(0.9, 0.5, 0.3);

        // 6: Left -> Delete
        p.slots[6].commandId = 12109;
        p.slots[6].customLabel = "Delete"_s;
        p.slots[6].accentColor = Vector(1.0, 0.3, 0.3); // Red

        // 7: Up-Left -> Close Poly Hole
        p.slots[7].commandId = 431000009;
        p.slots[7].customLabel = "Close Hole"_s;
        p.slots[7].accentColor = Vector(0.7, 0.4, 0.8);

        // Center -> Optimize
        p.slots[8].commandId = 1011167;
        p.slots[8].customLabel = "Optimize"_s;
        p.slots[8].accentColor = Vector(0.6, 0.6, 0.6);

        m_profiles.Append(p) iferr_ignore("Append");
    }

    // -------------------------------------------------------------
    // PROFILE 1: EDGES MODE
    // -------------------------------------------------------------
    {
        PieProfile p;
        p.name = "Edges"_s;
        p.context = PieContextMode::Edges;
        p.slotCount = 8;
        p.hotkey.enabled = true;
        p.hotkey.ctrl = true;
        p.hotkey.shift = true;
        p.hotkey.alt = false;
        p.hotkey.vkCode = 0;

        // 0: Up -> Bevel
        p.slots[0].commandId = 1011155;
        p.slots[0].customLabel = "Bevel"_s;
        p.slots[0].accentColor = Vector(0.25, 0.65, 1.0);

        // 1: Up-Right -> Slide
        p.slots[1].commandId = 1011160;
        p.slots[1].customLabel = "Slide"_s;
        p.slots[1].accentColor = Vector(0.2, 0.8, 0.7);

        // 2: Right -> Dissolve
        p.slots[2].commandId = 1011162;
        p.slots[2].customLabel = "Dissolve"_s;
        p.slots[2].accentColor = Vector(0.4, 0.6, 0.9);

        // 3: Down-Right -> Bridge
        p.slots[3].commandId = 431000010;
        p.slots[3].customLabel = "Bridge"_s;
        p.slots[3].accentColor = Vector(0.5, 0.5, 1.0);

        // 4: Down -> Loop Cut
        p.slots[4].commandId = 1038596;
        p.slots[4].customLabel = "Loop Cut"_s;
        p.slots[4].accentColor = Vector(1.0, 0.75, 0.2);

        // 5: Down-Left -> Edge Cut
        p.slots[5].commandId = 1011158;
        p.slots[5].customLabel = "Edge Cut"_s;
        p.slots[5].accentColor = Vector(0.9, 0.5, 0.3);

        // 6: Left -> Delete
        p.slots[6].commandId = 12109;
        p.slots[6].customLabel = "Delete"_s;
        p.slots[6].accentColor = Vector(1.0, 0.3, 0.3);

        // 7: Up-Left -> Spin Edge
        p.slots[7].commandId = 431000012;
        p.slots[7].customLabel = "Spin Edge"_s;
        p.slots[7].accentColor = Vector(0.7, 0.4, 0.8);

        // Center -> Extrude
        p.slots[8].commandId = 1011152;
        p.slots[8].customLabel = "Extrude"_s;
        p.slots[8].accentColor = Vector(0.6, 0.6, 0.6);

        m_profiles.Append(p) iferr_ignore("Append");
    }

    // -------------------------------------------------------------
    // PROFILE 2: POINTS MODE
    // -------------------------------------------------------------
    {
        PieProfile p;
        p.name = "Points"_s;
        p.context = PieContextMode::Points;
        p.slotCount = 8;
        p.hotkey.enabled = true;
        p.hotkey.ctrl = true;
        p.hotkey.shift = true;
        p.hotkey.alt = false;
        p.hotkey.vkCode = 0;

        // 0: Up -> Optimize
        p.slots[0].commandId = 1011167;
        p.slots[0].customLabel = "Optimize"_s;
        p.slots[0].accentColor = Vector(0.15, 0.85, 0.45);

        // 1: Up-Right -> Weld
        p.slots[1].commandId = 1011165;
        p.slots[1].customLabel = "Weld"_s;
        p.slots[1].accentColor = Vector(0.2, 0.8, 0.7);

        // 2: Right -> Slide
        p.slots[2].commandId = 1011160;
        p.slots[2].customLabel = "Slide"_s;
        p.slots[2].accentColor = Vector(0.25, 0.65, 1.0);

        // 3: Down-Right -> Iron
        p.slots[3].commandId = 1011163;
        p.slots[3].customLabel = "Iron"_s;
        p.slots[3].accentColor = Vector(0.5, 0.5, 1.0);

        // 4: Down -> Set Point Value
        p.slots[4].commandId = 1011168;
        p.slots[4].customLabel = "Set Value"_s;
        p.slots[4].accentColor = Vector(1.0, 0.75, 0.2);

        // 5: Down-Left -> Bevel
        p.slots[5].commandId = 1011155;
        p.slots[5].customLabel = "Bevel Point"_s;
        p.slots[5].accentColor = Vector(0.9, 0.5, 0.3);

        // 6: Left -> Delete
        p.slots[6].commandId = 12109;
        p.slots[6].customLabel = "Delete"_s;
        p.slots[6].accentColor = Vector(1.0, 0.3, 0.3);

        // 7: Up-Left -> Bridge
        p.slots[7].commandId = 431000010;
        p.slots[7].customLabel = "Bridge"_s;
        p.slots[7].accentColor = Vector(0.7, 0.4, 0.8);

        // Center -> Dissolve
        p.slots[8].commandId = 1011162;
        p.slots[8].customLabel = "Dissolve"_s;
        p.slots[8].accentColor = Vector(0.6, 0.6, 0.6);

        m_profiles.Append(p) iferr_ignore("Append");
    }

    // -------------------------------------------------------------
    // PROFILE 3: OBJECTS MODE
    // -------------------------------------------------------------
    {
        PieProfile p;
        p.name = "Objects"_s;
        p.context = PieContextMode::Objects;
        p.slotCount = 8;
        p.hotkey.enabled = true;
        p.hotkey.ctrl = true;
        p.hotkey.shift = true;
        p.hotkey.alt = false;
        p.hotkey.vkCode = 0;

        // 0: Up -> Group Objects (Alt+G)
        p.slots[0].commandId = 12141;
        p.slots[0].customLabel = "Group (Null)"_s;
        p.slots[0].accentColor = Vector(0.25, 0.65, 1.0);

        // 1: Up-Right -> Subdivision Surface
        p.slots[1].commandId = 1007455;
        p.slots[1].customLabel = "Subdivision"_s;
        p.slots[1].accentColor = Vector(0.2, 0.85, 0.5);

        // 2: Right -> Connect Objects + Delete
        p.slots[2].commandId = 12144;
        p.slots[2].customLabel = "Connect+Del"_s;
        p.slots[2].accentColor = Vector(0.3, 0.7, 0.9);

        // 3: Down-Right -> Boole
        p.slots[3].commandId = 1007457;
        p.slots[3].customLabel = "Boole"_s;
        p.slots[3].accentColor = Vector(0.5, 0.5, 1.0);

        // 4: Down -> Remesh
        p.slots[4].commandId = 1054321;
        p.slots[4].customLabel = "Remesh"_s;
        p.slots[4].accentColor = Vector(1.0, 0.75, 0.2);

        // 5: Down-Left -> Symmetry
        p.slots[5].commandId = 1007456;
        p.slots[5].customLabel = "Symmetry"_s;
        p.slots[5].accentColor = Vector(0.9, 0.5, 0.3);

        // 6: Left -> Delete
        p.slots[6].commandId = 12109;
        p.slots[6].customLabel = "Delete"_s;
        p.slots[6].accentColor = Vector(1.0, 0.3, 0.3);

        // 7: Up-Left -> Ungroup
        p.slots[7].commandId = 12142;
        p.slots[7].customLabel = "Ungroup"_s;
        p.slots[7].accentColor = Vector(0.7, 0.4, 0.8);

        // Center -> Current State to Object
        p.slots[8].commandId = 12233;
        p.slots[8].customLabel = "Current State"_s;
        p.slots[8].accentColor = Vector(0.6, 0.6, 0.6);

        m_profiles.Append(p) iferr_ignore("Append");
    }

    // -------------------------------------------------------------
    // PROFILE 4: GLOBAL / SELECTION (FALLBACK)
    // -------------------------------------------------------------
    {
        PieProfile p;
        p.name = "Global"_s;
        p.context = PieContextMode::Global;
        p.slotCount = 4;
        p.hotkey.enabled = true;
        p.hotkey.ctrl = true;
        p.hotkey.shift = true;
        p.hotkey.alt = false;
        p.hotkey.vkCode = 0;

        // 0: Up -> Live Selection
        p.slots[0].commandId = 200000083;
        p.slots[0].customLabel = "Live Select"_s;
        p.slots[0].accentColor = Vector(0.15, 0.85, 0.45);

        // 1: Right -> Move Tool
        p.slots[1].commandId = 200000088;
        p.slots[1].customLabel = "Move Tool"_s;
        p.slots[1].accentColor = Vector(0.25, 0.65, 1.0);

        // 2: Down -> Scale Tool
        p.slots[2].commandId = 200000089;
        p.slots[2].customLabel = "Scale Tool"_s;
        p.slots[2].accentColor = Vector(1.0, 0.75, 0.2);

        // 3: Left -> Rotate Tool
        p.slots[3].commandId = 200000090;
        p.slots[3].customLabel = "Rotate Tool"_s;
        p.slots[3].accentColor = Vector(1.0, 0.3, 0.3);

        // Center -> Undo
        p.slots[8].commandId = 12105;
        p.slots[8].customLabel = "Undo"_s;
        p.slots[8].accentColor = Vector(0.6, 0.6, 0.6);

        m_profiles.Append(p) iferr_ignore("Append");
    }
}

Filename PieProfileManager::GetConfigPath() const
{
    Filename path = GeGetC4DPath(C4D_PATH_PREFS);
    path = path + Filename("piemenu_profiles.dat"_s);
    return path;
}

Bool PieProfileManager::LoadFromFile()
{
    Filename path = GetConfigPath();
    AutoAlloc<BaseFile> file;
    if (!file || !file->Open(path, FILEOPEN::READ))
    {
        if (m_profiles.IsEmpty())
            InitDefaults();
        return false;
    }

    Int32 firstVal = 0;
    if (!file->ReadInt32(&firstVal))
    {
        if (m_profiles.IsEmpty())
            InitDefaults();
        return false;
    }

    Int32 profileCount = 0;
    Bool isV2 = false;

    if (firstVal == PIE_CONFIG_MAGIC_V2)
    {
        isV2 = true;
        if (!file->ReadInt32(&profileCount) || profileCount <= 0 || profileCount > 1000)
        {
            if (m_profiles.IsEmpty())
                InitDefaults();
            return false;
        }
    }
    else if (firstVal > 0 && firstVal <= 1000)
    {
        isV2 = false;
        profileCount = firstVal;
    }
    else
    {
        if (m_profiles.IsEmpty())
            InitDefaults();
        return false;
    }

    m_profiles.Reset();
    for (Int32 p = 0; p < profileCount; ++p)
    {
        PieProfile prof;
        Int32 ctx = 0;
        file->ReadString(&prof.name);
        file->ReadInt32(&ctx);
        prof.context = (PieContextMode)ctx;
        file->ReadInt32(&prof.slotCount);

        for (Int32 s = 0; s < 9; ++s)
        {
            file->ReadInt32(&prof.slots[s].commandId);
            file->ReadString(&prof.slots[s].customLabel);
            file->ReadVector64(&prof.slots[s].accentColor);
            file->ReadBool(&prof.slots[s].enabled);
        }

        if (isV2)
        {
            file->ReadBool(&prof.hotkey.enabled);
            file->ReadBool(&prof.hotkey.ctrl);
            file->ReadBool(&prof.hotkey.shift);
            file->ReadBool(&prof.hotkey.alt);
            file->ReadInt32(&prof.hotkey.vkCode);
        }
        else
        {
            // Backward-compatibility: initialize with default Ctrl+Shift
            prof.hotkey.enabled = true;
            prof.hotkey.ctrl = true;
            prof.hotkey.shift = true;
            prof.hotkey.alt = false;
            prof.hotkey.vkCode = 0;
        }

        m_profiles.Append(prof) iferr_ignore("Append");
    }

    file->ReadBool(&m_useCtrlShift);
    file->ReadInt32(&m_triggerKey);
    file->ReadFloat64(&m_radius);
    file->ReadFloat64(&m_deadzone);

    file->Close();

    if (m_profiles.IsEmpty())
    {
        InitDefaults();
    }

    return true;
}

Bool PieProfileManager::SaveToFile()
{
    Filename path = GetConfigPath();
    AutoAlloc<BaseFile> file;
    if (!file || !file->Open(path, FILEOPEN::WRITE))
        return false;

    // Write magic V2 header to denote per-profile hotkey format
    file->WriteInt32(PIE_CONFIG_MAGIC_V2);

    Int32 profileCount = (Int32)m_profiles.GetCount();
    file->WriteInt32(profileCount);

    for (Int32 p = 0; p < profileCount; ++p)
    {
        const PieProfile& prof = m_profiles[p];
        file->WriteString(prof.name);
        file->WriteInt32((Int32)prof.context);
        file->WriteInt32(prof.slotCount);

        for (Int32 s = 0; s < 9; ++s)
        {
            file->WriteInt32(prof.slots[s].commandId);
            file->WriteString(prof.slots[s].customLabel);
            file->WriteVector64(prof.slots[s].accentColor);
            file->WriteBool(prof.slots[s].enabled);
        }

        file->WriteBool(prof.hotkey.enabled);
        file->WriteBool(prof.hotkey.ctrl);
        file->WriteBool(prof.hotkey.shift);
        file->WriteBool(prof.hotkey.alt);
        file->WriteInt32(prof.hotkey.vkCode);
    }

    file->WriteBool(m_useCtrlShift);
    file->WriteInt32(m_triggerKey);
    file->WriteFloat64(m_radius);
    file->WriteFloat64(m_deadzone);

    file->Close();
    return true;
}

Int32 PieProfileManager::GetProfileCount() const
{
    return (Int32)m_profiles.GetCount();
}

PieProfile* PieProfileManager::GetProfile(Int32 index)
{
    if (index >= 0 && index < (Int32)m_profiles.GetCount())
        return &m_profiles[index];
    return nullptr;
}

Int32 PieProfileManager::GetActiveProfileIndex(BaseDocument* doc)
{
    if (m_profiles.IsEmpty())
        return 0;

    PieContextMode desired = GetContextModeForDoc(doc);

    // 1. Try exact context match
    if (desired != PieContextMode::Global)
    {
        for (Int32 i = 0; i < (Int32)m_profiles.GetCount(); ++i)
        {
            if (m_profiles[i].context == desired)
                return i;
        }
    }

    // 2. Fallback to Global context profile
    for (Int32 i = 0; i < (Int32)m_profiles.GetCount(); ++i)
    {
        if (m_profiles[i].context == PieContextMode::Global)
            return i;
    }

    return 0; // Fallback to first profile
}

PieProfile* PieProfileManager::GetActiveProfileForDoc(BaseDocument* doc)
{
    Int32 idx = GetActiveProfileIndex(doc);
    return GetProfile(idx);
}

Int32 PieProfileManager::AddProfile(const maxon::String& name, PieContextMode context, Int32 slotCount)
{
    PieProfile p;
    p.name = name.IsPopulated() ? name : FormatString("Menu @"_s, (Int32)m_profiles.GetCount() + 1);
    p.context = context;
    p.slotCount = (slotCount == 4) ? 4 : 8;
    p.hotkey.enabled = false;
    p.hotkey.ctrl = false;
    p.hotkey.shift = false;
    p.hotkey.alt = false;
    p.hotkey.vkCode = 0;

    for (Int32 s = 0; s < 9; ++s)
    {
        p.slots[s].commandId = NOTOK;
        p.slots[s].customLabel = ""_s;
        p.slots[s].accentColor = Vector(0.25, 0.65, 1.0);
        p.slots[s].enabled = true;
    }

    m_profiles.Append(p) iferr_ignore("Append");
    return (Int32)m_profiles.GetCount() - 1;
}

Int32 PieProfileManager::DuplicateProfile(Int32 sourceIndex)
{
    if (sourceIndex < 0 || sourceIndex >= (Int32)m_profiles.GetCount())
        return NOTOK;

    PieProfile copy = m_profiles[sourceIndex];
    copy.name = FormatString("@ (Copy)"_s, copy.name);
    m_profiles.Append(copy) iferr_ignore("Append");
    return (Int32)m_profiles.GetCount() - 1;
}

Bool PieProfileManager::DeleteProfile(Int32 index)
{
    if (m_profiles.GetCount() <= 1)
        return false;

    if (index < 0 || index >= (Int32)m_profiles.GetCount())
        return false;

    m_profiles.Erase(index) iferr_ignore("Erase");
    return true;
}

Bool PieProfileManager::RenameProfile(Int32 index, const maxon::String& newName)
{
    if (index < 0 || index >= (Int32)m_profiles.GetCount() || !newName.IsPopulated())
        return false;

    m_profiles[index].name = newName;
    return true;
}

Bool PieProfileManager::SetProfileContext(Int32 index, PieContextMode context)
{
    if (index < 0 || index >= (Int32)m_profiles.GetCount())
        return false;

    m_profiles[index].context = context;
    return true;
}

PieProfile* PieProfileManager::FindTriggeredProfile(BaseDocument* doc) const
{
    Int32 idx = FindTriggeredProfileIndex(doc);
    if (idx >= 0 && idx < (Int32)m_profiles.GetCount())
        return const_cast<PieProfile*>(&m_profiles[idx]);
    return nullptr;
}

Int32 PieProfileManager::FindTriggeredProfileIndex(BaseDocument* doc) const
{
    maxon::BaseArray<Int32> matched;
    for (Int32 i = 0; i < (Int32)m_profiles.GetCount(); ++i)
    {
        if (m_profiles[i].hotkey.IsPhysicallyDown())
        {
            matched.Append(i) iferr_ignore("Append");
        }
    }

    if (matched.IsEmpty())
        return NOTOK;

    PieContextMode desired = GetContextModeForDoc(doc);

    // 1. Search for exact context match
    if (desired != PieContextMode::Global)
    {
        for (Int32 idx : matched)
        {
            if (m_profiles[idx].context == desired)
                return idx;
        }
    }

    // 2. Search for Global context match
    for (Int32 idx : matched)
    {
        if (m_profiles[idx].context == PieContextMode::Global)
            return idx;
    }

    // If all matched profiles have specific contexts that do NOT match the current mode, do not trigger
    return NOTOK;
}

Bool PieProfileManager::IsProfileHotkeyHeld(const PieProfile* prof) const
{
    return prof ? prof->hotkey.IsPhysicallyDown() : false;
}

Bool PieProfileManager::IsTriggerHeld() const
{
    if (m_useCtrlShift)
    {
#if defined(MAXON_TARGET_WINDOWS)
        SHORT ctrlState = GetAsyncKeyState(VK_CONTROL);
        SHORT shiftState = GetAsyncKeyState(VK_SHIFT);
        if ((ctrlState & 0x8000) && (shiftState & 0x8000))
            return true;
#endif
        BaseContainer keyState;
        if (GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_QUALIFIER, keyState))
        {
            Int32 qual = keyState.GetInt32(BFM_INPUT_QUALIFIER);
            if ((qual & QSHIFT) && (qual & QCTRL))
                return true;
        }
        return false;
    }
    else if (m_triggerKey > 0)
    {
        Int32 trigKey = m_triggerKey;
        if (trigKey >= 'a' && trigKey <= 'z')
            trigKey = toupper(trigKey);

#if defined(MAXON_TARGET_WINDOWS)
        if (trigKey >= 'A' && trigKey <= 'Z')
        {
            if (GetAsyncKeyState(trigKey) & 0x8000)
                return true;
        }
#endif
        BaseContainer keyState;
        if (GetInputState(BFM_INPUT_KEYBOARD, trigKey, keyState))
        {
            if (keyState.GetInt32(BFM_INPUT_VALUE) != 0)
                return true;
        }
    }
    return false;
}

} // namespace cinema
