#include "piemenu_data.h"
#include "piemenu_json.h"
#include <cmath>
#include "c4d_general.h"
#include "c4d_libs/lib_iconcollection.h"
#include "windows_include.h"

namespace cinema
{

static const Int32 PIE_CONFIG_MAGIC_V2 = 0x50494532; // "PIE2"
static const Int32 PIE_CONFIG_MAGIC_V3 = 0x50494533; // "PIE3"
static const Int32 PIE_CONFIG_MAGIC_V4 = 0x50494534; // "PIE4"
static const Int32 PIE_CONFIG_MAGIC_V5 = 0x50494535; // "PIE5"
static const Int32 PIE_CONFIG_MAGIC_V6 = 0x50494536; // "PIE6"
static const Int32 PIE_CONFIG_MAGIC_V7 = 0x50494537; // "PIE7"
static const Int32 PIE_CONFIG_MAGIC_V8 = 0x50494538; // "PIE8"
static const Int32 PIE_CONFIG_MAGIC_V9 = 0x50494539; // "PIE9"

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

    if (IsSubMenu())
        return subMenuName;

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

maxon::String PieSlot::GetEffectiveTitleForModifiers(Bool shift, Bool ctrl, Bool alt) const
{
    Int32 cmd = GetCommandForModifiers(shift, ctrl, alt);
    if (cmd > 0 && cmd != commandId)
    {
        String name = GetCommandName(cmd);
        if (name.IsPopulated())
            return name;
        return FormatString("Cmd @"_s, cmd);
    }
    return GetEffectiveTitle();
}

Bool PieSlot::GetIconBitmapForModifiers(Bool shift, Bool ctrl, Bool alt, IconData& outIcon) const
{
    Int32 cmd = GetCommandForModifiers(shift, ctrl, alt);
    if (cmd > 0)
    {
        return GetIcon(cmd, &outIcon);
    }
    return false;
}

Float PieProfile::GetSlotAngle(Int32 index) const
{
    Int32 count = slotCount;
    if (count < MIN_SLOT_COUNT) count = MIN_SLOT_COUNT;
    if (count > MAX_SLOT_COUNT) count = MAX_SLOT_COUNT;

    Float step = (2.0 * PI) / (Float)count;
    // Slot 0 is centered at Up (-PI * 0.5)
    Float angle = -PI * 0.5 + (Float)index * step;
    while (angle > PI)
        angle -= 2.0 * PI;
    while (angle <= -PI)
        angle += 2.0 * PI;
    return angle;
}

Int32 PieProfile::AngleToSlot(Float deg, Float dist, Float deadzone) const
{
    if (dist < deadzone)
        return NOTOK;

    Int32 count = slotCount;
    if (count < MIN_SLOT_COUNT) count = MIN_SLOT_COUNT;
    if (count > MAX_SLOT_COUNT) count = MAX_SLOT_COUNT;

    Float sliceDeg = 360.0 / (Float)count;
    Float halfSlice = sliceDeg * 0.5;

    // Slot 0 is centered at -90 degrees (Up / 12 o'clock).
    // The wedge for slot 0 spans [-90 - halfSlice, -90 + halfSlice).
    Float relDeg = deg - (-90.0 - halfSlice);
    while (relDeg < 0.0)
        relDeg += 360.0;
    while (relDeg >= 360.0)
        relDeg -= 360.0;

    Int32 slot = (Int32)(relDeg / sliceDeg);
    if (slot < 0) slot = 0;
    if (slot >= count) slot = count - 1;
    return slot;
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

    m_outerRadius = 112.0;
    m_innerRadius = 36.0;
    m_radius = 112.0;
    m_deadzone = 26.0;
    m_iconSize = 24.0;
    m_displayMode = PieDisplayMode::IconAndLabel;
}


Filename PieProfileManager::GetConfigPath() const
{
    Filename path = GeGetC4DPath(C4D_PATH_PREFS);
    path = path + Filename("piemenu_profiles.dat"_s);
    return path;
}

static std::string C4DStringToStdString(const maxon::String& str)
{
    if (str.IsEmpty())
        return "";
    maxon::BaseArray<maxon::Char> arr = str.GetCString() iferr_ignore("GetCString");
    if (arr.IsEmpty() || !arr.GetFirst())
        return "";
    return std::string(arr.GetFirst());
}

static maxon::String StdStringToC4DString(const std::string& str)
{
    if (str.empty())
        return maxon::String();
    return maxon::String(str.c_str());
}

static maxon::String GetSlotPositionName(Int32 slotCount, Int32 index)
{
    if (index == CENTER_SLOT_INDEX) return "Center"_s;
    if (slotCount == 4)
    {
        switch (index)
        {
            case 0: return "North"_s;
            case 1: return "East"_s;
            case 2: return "South"_s;
            case 3: return "West"_s;
            default: return "Unused"_s;
        }
    }
    else if (slotCount == 8)
    {
        switch (index)
        {
            case 0: return "North"_s;
            case 1: return "NorthEast"_s;
            case 2: return "East"_s;
            case 3: return "SouthEast"_s;
            case 4: return "South"_s;
            case 5: return "SouthWest"_s;
            case 6: return "West"_s;
            case 7: return "NorthWest"_s;
            default: return "Unused"_s;
        }
    }
    else
    {
        if (index >= 0 && index < slotCount)
        {
            Int32 deg = (Int32)(index * 360.0 / slotCount);
            return FormatString("Slot @ (@deg)"_s, index + 1, deg);
        }
        return "Unused"_s;
    }
}

static JsonValue ProfileToJson(const PieProfile& prof)
{
    JsonValue j = JsonValue::Object();
    j.Set("name", C4DStringToStdString(prof.name));
    j.Set("context", C4DStringToStdString(GetContextShortName(prof.context)));
    j.Set("slotCount", (int64_t)prof.slotCount);
    j.Set("showTitle", (bool)prof.showTitle);
    j.Set("outerRadius", (double)prof.outerRadius);
    j.Set("innerRadius", (double)prof.innerRadius);
    j.Set("radius", (double)prof.outerRadius);
    j.Set("deadzone", (double)prof.deadzone);
    j.Set("iconSize", (double)prof.iconSize);
    j.Set("displayMode", (int64_t)prof.displayMode);

    // Hotkey
    JsonValue jHk = JsonValue::Object();
    jHk.Set("enabled", (bool)prof.hotkey.enabled);
    jHk.Set("ctrl", (bool)prof.hotkey.ctrl);
    jHk.Set("shift", (bool)prof.hotkey.shift);
    jHk.Set("alt", (bool)prof.hotkey.alt);
    jHk.Set("vkCode", (int64_t)prof.hotkey.vkCode);
    jHk.Set("display", C4DStringToStdString(prof.hotkey.GetDisplayString()));
    j.Set("hotkey", jHk);

    // Slots
    JsonValue jSlots = JsonValue::Array();
    for (Int32 s = 0; s < prof.slotCount; ++s)
    {
        const PieSlot& slot = prof.slots[s];
        JsonValue jSlot = JsonValue::Object();
        jSlot.Set("index", (int64_t)s);
        jSlot.Set("position", C4DStringToStdString(GetSlotPositionName(prof.slotCount, s)));
        jSlot.Set("commandId", (int64_t)slot.commandId);
        if (slot.subMenuName.IsPopulated())
            jSlot.Set("subMenuName", C4DStringToStdString(slot.subMenuName));
        if (slot.commandId > 0)
        {
            String cname = GetCommandName(slot.commandId);
            if (cname.IsPopulated())
                jSlot.Set("commandName", C4DStringToStdString(cname));
        }
        if (slot.customLabel.IsPopulated())
            jSlot.Set("customLabel", C4DStringToStdString(slot.customLabel));
        if (slot.shiftCommandId > 0)
            jSlot.Set("shiftCommandId", (int64_t)slot.shiftCommandId);
        if (slot.ctrlCommandId > 0)
            jSlot.Set("ctrlCommandId", (int64_t)slot.ctrlCommandId);
        if (slot.altCommandId > 0)
            jSlot.Set("altCommandId", (int64_t)slot.altCommandId);

        JsonValue jCol = JsonValue::Array();
        jCol.Push(slot.accentColor.x);
        jCol.Push(slot.accentColor.y);
        jCol.Push(slot.accentColor.z);
        jSlot.Set("accentColor", jCol);

        jSlot.Set("enabled", (bool)slot.enabled);
        jSlots.Push(jSlot);
    }

    // Center Hub
    {
        const PieSlot& slot = prof.slots[CENTER_SLOT_INDEX];
        JsonValue jSlot = JsonValue::Object();
        jSlot.Set("index", (int64_t)CENTER_SLOT_INDEX);
        jSlot.Set("position", "Center");
        jSlot.Set("commandId", (int64_t)slot.commandId);
        if (slot.subMenuName.IsPopulated())
            jSlot.Set("subMenuName", C4DStringToStdString(slot.subMenuName));
        if (slot.commandId > 0)
        {
            String cname = GetCommandName(slot.commandId);
            if (cname.IsPopulated())
                jSlot.Set("commandName", C4DStringToStdString(cname));
        }
        if (slot.customLabel.IsPopulated())
            jSlot.Set("customLabel", C4DStringToStdString(slot.customLabel));
        if (slot.shiftCommandId > 0)
            jSlot.Set("shiftCommandId", (int64_t)slot.shiftCommandId);
        if (slot.ctrlCommandId > 0)
            jSlot.Set("ctrlCommandId", (int64_t)slot.ctrlCommandId);
        if (slot.altCommandId > 0)
            jSlot.Set("altCommandId", (int64_t)slot.altCommandId);

        JsonValue jCol = JsonValue::Array();
        jCol.Push(slot.accentColor.x);
        jCol.Push(slot.accentColor.y);
        jCol.Push(slot.accentColor.z);
        jSlot.Set("accentColor", jCol);

        jSlot.Set("enabled", (bool)slot.enabled);
        jSlots.Push(jSlot);
    }
    j.Set("slots", jSlots);

    return j;
}

static Bool ProfileFromJson(const JsonValue& jProf, PieProfile& outProf)
{
    if (!jProf.IsObject())
        return false;

    std::string nameStr = jProf.GetString("name", "Imported Menu");
    outProf.name = StdStringToC4DString(nameStr);

    std::string ctxStr = jProf.GetString("context", "Global");
    if (ctxStr == "Polygons") outProf.context = PieContextMode::Polygons;
    else if (ctxStr == "Edges") outProf.context = PieContextMode::Edges;
    else if (ctxStr == "Points") outProf.context = PieContextMode::Points;
    else if (ctxStr == "Objects") outProf.context = PieContextMode::Objects;
    else if (ctxStr == "UV") outProf.context = PieContextMode::UV;
    else if (jProf.Find("context") && jProf.Find("context")->IsNumber())
        outProf.context = (PieContextMode)(Int32)jProf.GetInt("context", 0);
    else
        outProf.context = PieContextMode::Global;

    int sc = (int)jProf.GetInt("slotCount", 8);
    if (sc < MIN_SLOT_COUNT) sc = MIN_SLOT_COUNT;
    if (sc > MAX_SLOT_COUNT) sc = MAX_SLOT_COUNT;
    outProf.slotCount = sc;
    outProf.showTitle = jProf.GetBool("showTitle", true);
    if (jProf.Find("outerRadius"))
    {
        outProf.outerRadius = (Float)jProf.GetNumber("outerRadius", 112.0);
        outProf.innerRadius = (Float)jProf.GetNumber("innerRadius", 36.0);
    }
    else
    {
        Float r = (Float)jProf.GetNumber("radius", 85.0);
        if (r >= 80.0 && r <= 90.0)
        {
            outProf.outerRadius = 112.0;
            outProf.innerRadius = 36.0;
        }
        else
        {
            Float scale = (r > 20.0) ? (r / 85.0) : 1.0;
            outProf.outerRadius = 112.0 * scale;
            outProf.innerRadius = 36.0 * scale;
        }
    }
    outProf.radius = outProf.outerRadius;
    outProf.deadzone = (Float)jProf.GetNumber("deadzone", 26.0);
    outProf.iconSize = (Float)jProf.GetNumber("iconSize", 24.0);
    outProf.displayMode = (PieDisplayMode)(int)jProf.GetInt("displayMode", 0);

    const JsonValue* jHk = jProf.GetObject("hotkey");
    if (jHk)
    {
        outProf.hotkey.enabled = jHk->GetBool("enabled", true);
        outProf.hotkey.ctrl = jHk->GetBool("ctrl", true);
        outProf.hotkey.shift = jHk->GetBool("shift", true);
        outProf.hotkey.alt = jHk->GetBool("alt", false);
        outProf.hotkey.vkCode = (Int32)jHk->GetInt("vkCode", 0);
    }
    else
    {
        outProf.hotkey.enabled = true;
        outProf.hotkey.ctrl = true;
        outProf.hotkey.shift = true;
        outProf.hotkey.alt = false;
        outProf.hotkey.vkCode = 0;
    }

    for (int s = 0; s < TOTAL_SLOT_COUNT; ++s)
    {
        outProf.slots[s].commandId = NOTOK;
        outProf.slots[s].subMenuName = ""_s;
        outProf.slots[s].customLabel = ""_s;
        outProf.slots[s].accentColor = Vector(0.25, 0.65, 1.0);
        outProf.slots[s].enabled = true;
    }

    const std::vector<JsonValue>* arrSlots = jProf.GetArray("slots");
    if (arrSlots)
    {
        for (size_t i = 0; i < arrSlots->size(); ++i)
        {
            const JsonValue& jSlot = (*arrSlots)[i];
            if (!jSlot.IsObject()) continue;
            int idx = (int)jSlot.GetInt("index", (int64_t)i);
            std::string pos = jSlot.GetString("position", "");

            if (pos == "Center" || idx == CENTER_SLOT_INDEX || (idx == 8 && (arrSlots->size() == 9 || sc <= 8) && (idx >= sc || pos.find("Center") != std::string::npos)))
            {
                idx = CENTER_SLOT_INDEX;
            }

            if (idx >= 0 && idx < TOTAL_SLOT_COUNT)
            {
                outProf.slots[idx].commandId = (Int32)jSlot.GetInt("commandId", NOTOK);
                outProf.slots[idx].shiftCommandId = (Int32)jSlot.GetInt("shiftCommandId", NOTOK);
                outProf.slots[idx].ctrlCommandId = (Int32)jSlot.GetInt("ctrlCommandId", NOTOK);
                outProf.slots[idx].altCommandId = (Int32)jSlot.GetInt("altCommandId", NOTOK);
                std::string sub = jSlot.GetString("subMenuName", "");
                outProf.slots[idx].subMenuName = StdStringToC4DString(sub);
                std::string lbl = jSlot.GetString("customLabel", "");
                outProf.slots[idx].customLabel = StdStringToC4DString(lbl);
                outProf.slots[idx].enabled = jSlot.GetBool("enabled", true);

                const std::vector<JsonValue>* arrCol = jSlot.GetArray("accentColor");
                if (arrCol && arrCol->size() >= 3)
                {
                    outProf.slots[idx].accentColor = Vector(
                        (*arrCol)[0].numVal,
                        (*arrCol)[1].numVal,
                        (*arrCol)[2].numVal
                    );
                }
            }
        }
    }

    return true;
}

static Bool ReadProfilesFromBinaryFile(BaseFile* file, maxon::BaseArray<PieProfile>& outProfiles, Float& outOuterRadius, Float& outInnerRadius, Float& outRadius, Float& outDeadzone, Float& outIconSize, PieDisplayMode& outDisplayMode, Bool& outUseCtrlShift, Int32& outTriggerKey)
{
    if (!file)
        return false;

    Int32 firstVal = 0;
    if (!file->ReadInt32(&firstVal))
        return false;

    Int32 profileCount = 0;
    Int32 version = 1;

    if (firstVal == PIE_CONFIG_MAGIC_V9)
    {
        version = 9;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal == PIE_CONFIG_MAGIC_V8)
    {
        version = 8;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal == PIE_CONFIG_MAGIC_V7)
    {
        version = 7;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal == PIE_CONFIG_MAGIC_V6)
    {
        version = 6;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal == PIE_CONFIG_MAGIC_V5)
    {
        version = 5;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal == PIE_CONFIG_MAGIC_V4)
    {
        version = 4;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal == PIE_CONFIG_MAGIC_V3)
    {
        version = 3;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal == PIE_CONFIG_MAGIC_V2)
    {
        version = 2;
        if (!file->ReadInt32(&profileCount) || profileCount < 0 || profileCount > 1000)
            return false;
    }
    else if (firstVal >= 0 && firstVal <= 1000)
    {
        version = 1;
        profileCount = firstVal;
    }
    else
    {
        return false;
    }

    outProfiles.Reset();
    for (Int32 p = 0; p < profileCount; ++p)
    {
        PieProfile prof;
        Int32 ctx = 0;
        file->ReadString(&prof.name);
        file->ReadInt32(&ctx);
        prof.context = (PieContextMode)ctx;
        file->ReadInt32(&prof.slotCount);
        if (prof.slotCount < MIN_SLOT_COUNT) prof.slotCount = MIN_SLOT_COUNT;
        if (prof.slotCount > MAX_SLOT_COUNT) prof.slotCount = MAX_SLOT_COUNT;

        for (Int32 s = 0; s < TOTAL_SLOT_COUNT; ++s)
        {
            prof.slots[s].commandId = NOTOK;
            prof.slots[s].subMenuName = ""_s;
            prof.slots[s].customLabel = ""_s;
            prof.slots[s].accentColor = Vector(0.25, 0.65, 1.0);
            prof.slots[s].enabled = true;
        }

        if (version >= 8)
        {
            for (Int32 s = 0; s < TOTAL_SLOT_COUNT; ++s)
            {
                file->ReadInt32(&prof.slots[s].commandId);
                file->ReadString(&prof.slots[s].customLabel);
                file->ReadVector64(&prof.slots[s].accentColor);
                file->ReadBool(&prof.slots[s].enabled);
                file->ReadString(&prof.slots[s].subMenuName);
                file->ReadInt32(&prof.slots[s].shiftCommandId);
                file->ReadInt32(&prof.slots[s].ctrlCommandId);
                file->ReadInt32(&prof.slots[s].altCommandId);
            }
        }
        else if (version >= 7)
        {
            for (Int32 s = 0; s < TOTAL_SLOT_COUNT; ++s)
            {
                file->ReadInt32(&prof.slots[s].commandId);
                file->ReadString(&prof.slots[s].customLabel);
                file->ReadVector64(&prof.slots[s].accentColor);
                file->ReadBool(&prof.slots[s].enabled);
                file->ReadString(&prof.slots[s].subMenuName);
            }
        }
        else
        {
            for (Int32 s = 0; s < 9; ++s)
            {
                Int32 cmdId = NOTOK;
                String cLabel;
                Vector aCol;
                Bool en = true;
                String subName;

                file->ReadInt32(&cmdId);
                file->ReadString(&cLabel);
                file->ReadVector64(&aCol);
                file->ReadBool(&en);
                if (version >= 4)
                {
                    file->ReadString(&subName);
                }

                Int32 targetSlot = (s == 8) ? CENTER_SLOT_INDEX : s;
                prof.slots[targetSlot].commandId = cmdId;
                prof.slots[targetSlot].customLabel = cLabel;
                prof.slots[targetSlot].accentColor = aCol;
                prof.slots[targetSlot].enabled = en;
                prof.slots[targetSlot].subMenuName = subName;
            }
        }

        if (version >= 2)
        {
            file->ReadBool(&prof.hotkey.enabled);
            file->ReadBool(&prof.hotkey.ctrl);
            file->ReadBool(&prof.hotkey.shift);
            file->ReadBool(&prof.hotkey.alt);
            file->ReadInt32(&prof.hotkey.vkCode);
        }
        else
        {
            prof.hotkey.enabled = true;
            prof.hotkey.ctrl = true;
            prof.hotkey.shift = true;
            prof.hotkey.alt = false;
            prof.hotkey.vkCode = 0;
        }

        if (version >= 3)
        {
            file->ReadBool(&prof.showTitle);
        }
        else
        {
            prof.showTitle = true;
        }

        if (version >= 9)
        {
            file->ReadFloat64(&prof.outerRadius);
            file->ReadFloat64(&prof.innerRadius);
            prof.radius = prof.outerRadius;
            file->ReadFloat64(&prof.deadzone);
            file->ReadFloat64(&prof.iconSize);
            Int32 inDisp = 0;
            file->ReadInt32(&inDisp);
            prof.displayMode = (PieDisplayMode)inDisp;
        }
        else if (version >= 6)
        {
            Float oldRadius = 85.0;
            file->ReadFloat64(&oldRadius);
            if (oldRadius >= 80.0 && oldRadius <= 90.0)
            {
                prof.outerRadius = 112.0;
                prof.innerRadius = 36.0;
            }
            else
            {
                Float scale = (oldRadius > 20.0) ? (oldRadius / 85.0) : 1.0;
                prof.outerRadius = 112.0 * scale;
                prof.innerRadius = 36.0 * scale;
            }
            prof.radius = prof.outerRadius;
            file->ReadFloat64(&prof.deadzone);
            file->ReadFloat64(&prof.iconSize);
            Int32 inDisp = 0;
            file->ReadInt32(&inDisp);
            prof.displayMode = (PieDisplayMode)inDisp;
        }
        else if (version >= 5)
        {
            prof.outerRadius = 112.0;
            prof.innerRadius = 36.0;
            prof.radius = 112.0;
            prof.deadzone = 26.0;
            file->ReadFloat64(&prof.iconSize);
            Int32 inDisp = 0;
            file->ReadInt32(&inDisp);
            prof.displayMode = (PieDisplayMode)inDisp;
        }
        else
        {
            prof.outerRadius = 112.0;
            prof.innerRadius = 36.0;
            prof.radius = 112.0;
            prof.deadzone = 26.0;
            prof.iconSize = 24.0;
            prof.displayMode = PieDisplayMode::IconAndLabel;
        }

        outProfiles.Append(prof) iferr_ignore("Append");
    }

    file->ReadBool(&outUseCtrlShift);
    file->ReadInt32(&outTriggerKey);
    if (version >= 9)
    {
        file->ReadFloat64(&outOuterRadius);
        file->ReadFloat64(&outInnerRadius);
    }
    else
    {
        Float oldRadius = 85.0;
        file->ReadFloat64(&oldRadius);
        if (oldRadius >= 80.0 && oldRadius <= 90.0)
        {
            outOuterRadius = 112.0;
            outInnerRadius = 36.0;
        }
        else
        {
            Float scale = (oldRadius > 20.0) ? (oldRadius / 85.0) : 1.0;
            outOuterRadius = 112.0 * scale;
            outInnerRadius = 36.0 * scale;
        }
    }
    outRadius = outOuterRadius;
    file->ReadFloat64(&outDeadzone);

    Float inIconSize = 24.0;
    if (file->ReadFloat64(&inIconSize))
    {
        outIconSize = inIconSize;
        Int32 inDispMode = 0;
        if (file->ReadInt32(&inDispMode))
        {
            outDisplayMode = (PieDisplayMode)inDispMode;
        }
    }

    // For files prior to V6, populate each profile with the global radius and deadzone
    if (version < 6)
    {
        for (Int32 p = 0; p < (Int32)outProfiles.GetCount(); ++p)
        {
            outProfiles[p].outerRadius = outOuterRadius;
            outProfiles[p].innerRadius = outInnerRadius;
            outProfiles[p].radius = outOuterRadius;
            outProfiles[p].deadzone = outDeadzone;
        }
    }

    // For files prior to V5, populate each profile with the global iconSize and displayMode
    if (version < 5)
    {
        for (Int32 p = 0; p < (Int32)outProfiles.GetCount(); ++p)
        {
            outProfiles[p].iconSize = outIconSize;
            outProfiles[p].displayMode = outDisplayMode;
        }
    }

    return true;
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

    if (!ReadProfilesFromBinaryFile(file, m_profiles, m_outerRadius, m_innerRadius, m_radius, m_deadzone, m_iconSize, m_displayMode, m_useCtrlShift, m_triggerKey))
    {
        if (m_profiles.IsEmpty())
            InitDefaults();
        file->Close();
        return false;
    }

    file->Close();
    return true;
}

Bool PieProfileManager::ExportSettingsToFile(const Filename& path, Int32 singleProfileIndex) const
{
    JsonValue root = JsonValue::Object();
    root.Set("generator", "Cinema 4D Pie Menu Plugin");
    root.Set("version", (int64_t)1);

    if (singleProfileIndex >= 0 && singleProfileIndex < (Int32)m_profiles.GetCount())
    {
        root.Set("profile", ProfileToJson(m_profiles[singleProfileIndex]));
    }
    else
    {
        JsonValue jSettings = JsonValue::Object();
        jSettings.Set("outerRadius", m_outerRadius);
        jSettings.Set("innerRadius", m_innerRadius);
        jSettings.Set("radius", m_outerRadius);
        jSettings.Set("deadzone", m_deadzone);
        jSettings.Set("iconSize", m_iconSize);
        jSettings.Set("displayMode", (int64_t)m_displayMode);
        root.Set("settings", jSettings);

        JsonValue jProfiles = JsonValue::Array();
        for (Int32 i = 0; i < (Int32)m_profiles.GetCount(); ++i)
        {
            jProfiles.Push(ProfileToJson(m_profiles[i]));
        }
        root.Set("profiles", jProfiles);
    }

    std::string jsonStr = root.Dump(2);

    AutoAlloc<BaseFile> file;
    if (!file || !file->Open(path, FILEOPEN::WRITE))
        return false;

    Int64 written = file->WriteBytes(jsonStr.data(), (Int)jsonStr.size());
    file->Close();

    return (written == (Int64)jsonStr.size());
}

Bool PieProfileManager::InspectImportFile(const Filename& path, Int32& outProfileCount, maxon::String& outFirstName, Bool& outHasGlobalSettings) const
{
    outProfileCount = 0;
    outFirstName = ""_s;
    outHasGlobalSettings = false;

    AutoAlloc<BaseFile> file;
    if (!file || !file->Open(path, FILEOPEN::READ))
        return false;

    Int64 len = file->GetLength();
    if (len <= 0 || len > 20 * 1024 * 1024)
        return false;

    // Check if binary DAT format
    Int32 firstVal = 0;
    if (file->ReadInt32(&firstVal))
    {
        if (firstVal == PIE_CONFIG_MAGIC_V9 || firstVal == PIE_CONFIG_MAGIC_V8 || firstVal == PIE_CONFIG_MAGIC_V7 || firstVal == PIE_CONFIG_MAGIC_V6 || firstVal == PIE_CONFIG_MAGIC_V5 || firstVal == PIE_CONFIG_MAGIC_V4 || firstVal == PIE_CONFIG_MAGIC_V3 || firstVal == PIE_CONFIG_MAGIC_V2 || (firstVal > 0 && firstVal <= 1000))
        {
            file->Seek(0, FILESEEK::START);
            maxon::BaseArray<PieProfile> tempProfiles;
            Float tempOuterRadius = 112.0;
            Float tempInnerRadius = 36.0;
            Float tempRadius = 112.0;
            Float tempDeadzone = 26.0;
            Float tempIconSize = 24.0;
            PieDisplayMode tempDisplayMode = PieDisplayMode::IconAndLabel;
            Bool tempCtrlShift = false;
            Int32 tempTrigger = 'V';
            if (ReadProfilesFromBinaryFile(file, tempProfiles, tempOuterRadius, tempInnerRadius, tempRadius, tempDeadzone, tempIconSize, tempDisplayMode, tempCtrlShift, tempTrigger))
            {
                outProfileCount = (Int32)tempProfiles.GetCount();
                if (outProfileCount > 0)
                    outFirstName = tempProfiles[0].name;
                outHasGlobalSettings = true;
                return true;
            }
        }
    }

    // Try JSON format
    file->Seek(0, FILESEEK::START);
    std::string content((size_t)len, '\0');
    if (file->ReadBytes(&content[0], len) != len)
        return false;

    JsonValue root;
    std::string err;
    if (!JsonValue::Parse(content, root, err))
        return false;

    if (root.IsObject())
    {
        outHasGlobalSettings = root.Has("settings");
        if (root.Has("profile"))
        {
            const JsonValue* p = root.GetObject("profile");
            if (p)
            {
                outProfileCount = 1;
                outFirstName = StdStringToC4DString(p->GetString("name", "Imported Menu"));
                return true;
            }
        }
        const std::vector<JsonValue>* arr = root.GetArray("profiles");
        if (arr && !arr->empty())
        {
            outProfileCount = (Int32)arr->size();
            outFirstName = StdStringToC4DString((*arr)[0].GetString("name", "Imported Menu"));
            return true;
        }

        if (root.Has("slots"))
        {
            outProfileCount = 1;
            outFirstName = StdStringToC4DString(root.GetString("name", "Imported Menu"));
            return true;
        }
    }
    else if (root.IsArray() && !root.arrVal.empty())
    {
        outProfileCount = (Int32)root.arrVal.size();
        outFirstName = StdStringToC4DString(root.arrVal[0].GetString("name", "Imported Menu"));
        return true;
    }

    return false;
}

Bool PieProfileManager::ImportSettingsFromFile(const Filename& path, Bool replaceExisting, Int32& outImportedCount)
{
    outImportedCount = 0;

    AutoAlloc<BaseFile> file;
    if (!file || !file->Open(path, FILEOPEN::READ))
        return false;

    Int64 len = file->GetLength();
    if (len <= 0 || len > 20 * 1024 * 1024)
        return false;

    maxon::BaseArray<PieProfile> parsedProfiles;
    Float parsedOuterRadius = m_outerRadius;
    Float parsedInnerRadius = m_innerRadius;
    Float parsedDeadzone = m_deadzone;
    Float parsedIconSize = m_iconSize;
    PieDisplayMode parsedDisplayMode = m_displayMode;
    Bool hasParsedSettings = false;

    // 1. Try binary DAT format
    Int32 firstVal = 0;
    if (file->ReadInt32(&firstVal))
    {
        if (firstVal == PIE_CONFIG_MAGIC_V9 || firstVal == PIE_CONFIG_MAGIC_V8 || firstVal == PIE_CONFIG_MAGIC_V7 || firstVal == PIE_CONFIG_MAGIC_V6 || firstVal == PIE_CONFIG_MAGIC_V5 || firstVal == PIE_CONFIG_MAGIC_V4 || firstVal == PIE_CONFIG_MAGIC_V3 || firstVal == PIE_CONFIG_MAGIC_V2 || (firstVal > 0 && firstVal <= 1000))
        {
            file->Seek(0, FILESEEK::START);
            Float dummyRadius = m_radius;
            Bool dummyCtrlShift = false;
            Int32 dummyTrigger = 'V';
            if (ReadProfilesFromBinaryFile(file, parsedProfiles, parsedOuterRadius, parsedInnerRadius, dummyRadius, parsedDeadzone, parsedIconSize, parsedDisplayMode, dummyCtrlShift, dummyTrigger))
            {
                hasParsedSettings = true;
            }
        }
    }

    // 2. Try JSON format if not DAT
    if (parsedProfiles.IsEmpty())
    {
        file->Seek(0, FILESEEK::START);
        std::string content((size_t)len, '\0');
        if (file->ReadBytes(&content[0], len) != len)
            return false;

        JsonValue root;
        std::string err;
        if (!JsonValue::Parse(content, root, err))
            return false;

        if (root.IsObject())
        {
            const JsonValue* jSettings = root.GetObject("settings");
            if (jSettings)
            {
                if (jSettings->Find("outerRadius"))
                {
                    parsedOuterRadius = (Float)jSettings->GetNumber("outerRadius", (double)m_outerRadius);
                    parsedInnerRadius = (Float)jSettings->GetNumber("innerRadius", (double)m_innerRadius);
                }
                else if (jSettings->Find("radius"))
                {
                    Float r = (Float)jSettings->GetNumber("radius", 85.0);
                    if (r >= 80.0 && r <= 90.0)
                    {
                        parsedOuterRadius = 112.0;
                        parsedInnerRadius = 36.0;
                    }
                    else
                    {
                        Float scale = (r > 20.0) ? (r / 85.0) : 1.0;
                        parsedOuterRadius = 112.0 * scale;
                        parsedInnerRadius = 36.0 * scale;
                    }
                }
                parsedDeadzone = (Float)jSettings->GetNumber("deadzone", (double)m_deadzone);
                parsedIconSize = (Float)jSettings->GetNumber("iconSize", (double)m_iconSize);
                parsedDisplayMode = (PieDisplayMode)jSettings->GetInt("displayMode", (int64_t)m_displayMode);
                hasParsedSettings = true;
            }

            if (root.Has("profile"))
            {
                const JsonValue* p = root.GetObject("profile");
                if (p)
                {
                    PieProfile prof;
                    if (ProfileFromJson(*p, prof))
                        parsedProfiles.Append(prof) iferr_ignore("Append");
                }
            }
            else if (root.Has("profiles"))
            {
                const std::vector<JsonValue>* arr = root.GetArray("profiles");
                if (arr)
                {
                    for (size_t i = 0; i < arr->size(); ++i)
                    {
                        PieProfile prof;
                        if (ProfileFromJson((*arr)[i], prof))
                            parsedProfiles.Append(prof) iferr_ignore("Append");
                    }
                }
            }
            else if (root.Has("slots"))
            {
                PieProfile prof;
                if (ProfileFromJson(root, prof))
                    parsedProfiles.Append(prof) iferr_ignore("Append");
            }
        }
        else if (root.IsArray())
        {
            for (size_t i = 0; i < root.arrVal.size(); ++i)
            {
                PieProfile prof;
                if (ProfileFromJson(root.arrVal[i], prof))
                    parsedProfiles.Append(prof) iferr_ignore("Append");
            }
        }
    }

    if (parsedProfiles.IsEmpty())
        return false;

    outImportedCount = (Int32)parsedProfiles.GetCount();

    if (replaceExisting)
    {
        m_profiles.Reset();
        for (Int32 i = 0; i < (Int32)parsedProfiles.GetCount(); ++i)
        {
            if (hasParsedSettings)
            {
                if (parsedProfiles[i].outerRadius == 112.0 && parsedOuterRadius != 112.0)
                {
                    parsedProfiles[i].outerRadius = parsedOuterRadius;
                    parsedProfiles[i].radius = parsedOuterRadius;
                }
                if (parsedProfiles[i].innerRadius == 36.0 && parsedInnerRadius != 36.0)
                    parsedProfiles[i].innerRadius = parsedInnerRadius;
                if (parsedProfiles[i].deadzone == 26.0 && parsedDeadzone != 26.0)
                    parsedProfiles[i].deadzone = parsedDeadzone;
            }
            m_profiles.Append(parsedProfiles[i]) iferr_ignore("Append");
        }
        if (hasParsedSettings)
        {
            m_outerRadius = parsedOuterRadius;
            m_innerRadius = parsedInnerRadius;
            m_radius = parsedOuterRadius;
            m_deadzone = parsedDeadzone;
            m_iconSize = parsedIconSize;
            m_displayMode = parsedDisplayMode;
        }
    }
    else
    {
        // Append
        for (Int32 i = 0; i < (Int32)parsedProfiles.GetCount(); ++i)
        {
            if (hasParsedSettings)
            {
                if (parsedProfiles[i].outerRadius == 112.0 && parsedOuterRadius != 112.0)
                {
                    parsedProfiles[i].outerRadius = parsedOuterRadius;
                    parsedProfiles[i].radius = parsedOuterRadius;
                }
                if (parsedProfiles[i].innerRadius == 36.0 && parsedInnerRadius != 36.0)
                    parsedProfiles[i].innerRadius = parsedInnerRadius;
                if (parsedProfiles[i].deadzone == 26.0 && parsedDeadzone != 26.0)
                    parsedProfiles[i].deadzone = parsedDeadzone;
            }
            m_profiles.Append(parsedProfiles[i]) iferr_ignore("Append");
        }
    }

    return true;
}

Bool PieProfileManager::SaveToFile()
{
    Filename path = GetConfigPath();
    AutoAlloc<BaseFile> file;
    if (!file || !file->Open(path, FILEOPEN::WRITE))
        return false;

    // Write magic V9 header to denote separate outer and inner radius support
    file->WriteInt32(PIE_CONFIG_MAGIC_V9);

    Int32 profileCount = (Int32)m_profiles.GetCount();
    file->WriteInt32(profileCount);

    for (Int32 p = 0; p < profileCount; ++p)
    {
        const PieProfile& prof = m_profiles[p];
        file->WriteString(prof.name);
        file->WriteInt32((Int32)prof.context);
        file->WriteInt32(prof.slotCount);

        for (Int32 s = 0; s < TOTAL_SLOT_COUNT; ++s)
        {
            file->WriteInt32(prof.slots[s].commandId);
            file->WriteString(prof.slots[s].customLabel);
            file->WriteVector64(prof.slots[s].accentColor);
            file->WriteBool(prof.slots[s].enabled);
            file->WriteString(prof.slots[s].subMenuName);
            file->WriteInt32(prof.slots[s].shiftCommandId);
            file->WriteInt32(prof.slots[s].ctrlCommandId);
            file->WriteInt32(prof.slots[s].altCommandId);
        }

        file->WriteBool(prof.hotkey.enabled);
        file->WriteBool(prof.hotkey.ctrl);
        file->WriteBool(prof.hotkey.shift);
        file->WriteBool(prof.hotkey.alt);
        file->WriteInt32(prof.hotkey.vkCode);

        file->WriteBool(prof.showTitle);

        file->WriteFloat64(prof.outerRadius);
        file->WriteFloat64(prof.innerRadius);
        file->WriteFloat64(prof.deadzone);
        file->WriteFloat64(prof.iconSize);
        file->WriteInt32((Int32)prof.displayMode);
    }

    file->WriteBool(m_useCtrlShift);
    file->WriteInt32(m_triggerKey);
    file->WriteFloat64(m_outerRadius);
    file->WriteFloat64(m_innerRadius);
    file->WriteFloat64(m_deadzone);
    file->WriteFloat64(m_iconSize);
    file->WriteInt32((Int32)m_displayMode);

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

PieProfile* PieProfileManager::FindProfileByName(const maxon::String& name)
{
    if (name.IsEmpty())
        return nullptr;

    for (Int32 i = 0; i < (Int32)m_profiles.GetCount(); ++i)
    {
        if (m_profiles[i].name == name)
            return &m_profiles[i];
    }

    String searchName(name);
    for (Int32 i = 0; i < (Int32)m_profiles.GetCount(); ++i)
    {
        if (String(m_profiles[i].name).LexCompare(searchName) == 0)
            return &m_profiles[i];
    }

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
    if (slotCount < MIN_SLOT_COUNT) slotCount = MIN_SLOT_COUNT;
    if (slotCount > MAX_SLOT_COUNT) slotCount = MAX_SLOT_COUNT;
    p.slotCount = slotCount;
    p.hotkey.enabled = false;
    p.hotkey.ctrl = false;
    p.hotkey.shift = false;
    p.hotkey.alt = false;
    p.hotkey.vkCode = 0;

    for (Int32 s = 0; s < TOTAL_SLOT_COUNT; ++s)
    {
        p.slots[s].commandId = NOTOK;
        p.slots[s].subMenuName = ""_s;
        p.slots[s].customLabel = ""_s;
        p.slots[s].accentColor = Vector(0.25, 0.65, 1.0);
        p.slots[s].enabled = true;
    }

    p.outerRadius = m_outerRadius;
    p.innerRadius = m_innerRadius;
    p.radius = m_outerRadius;
    p.deadzone = m_deadzone;
    p.iconSize = m_iconSize;
    p.displayMode = m_displayMode;

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
    if (m_profiles.IsEmpty())
        return false;

    if (index < 0 || index >= (Int32)m_profiles.GetCount())
        return false;

    maxon::String delName = m_profiles[index].name;
    m_profiles.Erase(index) iferr_ignore("Erase");

    // Clear subMenuName in any slot referencing the deleted profile
    for (Int32 p = 0; p < (Int32)m_profiles.GetCount(); ++p)
    {
        for (Int32 s = 0; s < TOTAL_SLOT_COUNT; ++s)
        {
            if (m_profiles[p].slots[s].subMenuName == delName)
            {
                m_profiles[p].slots[s].subMenuName = ""_s;
            }
        }
    }

    return true;
}

Bool PieProfileManager::RenameProfile(Int32 index, const maxon::String& newName)
{
    if (index < 0 || index >= (Int32)m_profiles.GetCount() || !newName.IsPopulated())
        return false;

    maxon::String oldName = m_profiles[index].name;
    m_profiles[index].name = newName;

    // Update references across all profiles' slots
    for (Int32 p = 0; p < (Int32)m_profiles.GetCount(); ++p)
    {
        for (Int32 s = 0; s < TOTAL_SLOT_COUNT; ++s)
        {
            if (m_profiles[p].slots[s].subMenuName == oldName)
            {
                m_profiles[p].slots[s].subMenuName = newName;
                if (m_profiles[p].slots[s].customLabel == oldName)
                    m_profiles[p].slots[s].customLabel = newName;
            }
        }
    }

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

    // 3. Unique hotkey match: if only one profile matched the pressed shortcut, trigger it regardless of mode
    if (matched.GetCount() == 1)
        return matched[0];

    // If all matched profiles have specific contexts that do NOT match the current mode, do not trigger
    return NOTOK;
}

Bool PieProfileManager::IsProfileHotkeyHeld(const PieProfile* prof) const
{
    return prof ? prof->hotkey.IsPhysicallyDown() : false;
}

Bool PieProfileManager::IsTriggerHeld() const
{
    // Deprecated global check: trigger is now per-profile
    return false;
}

} // namespace cinema
