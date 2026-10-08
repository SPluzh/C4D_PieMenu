#include "piemenu_data.h"
#include <cmath>
#include "c4d_general.h"
#include "c4d_libs/lib_iconcollection.h"

namespace cinema
{

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
        return false;

    Int32 profileCount = 0;
    if (!file->ReadInt32(&profileCount) || profileCount <= 0 || profileCount > 32)
        return false;

    m_profiles.Reset();
    for (Int32 p = 0; p < profileCount; ++p)
    {
        PieProfile prof;
        String name;
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
        m_profiles.Append(prof) iferr_ignore("Append");
    }

    file->ReadBool(&m_useCtrlShift);
    file->ReadInt32(&m_triggerKey);
    file->ReadFloat64(&m_radius);
    file->ReadFloat64(&m_deadzone);

    file->Close();
    return true;
}

Bool PieProfileManager::SaveToFile()
{
    Filename path = GetConfigPath();
    AutoAlloc<BaseFile> file;
    if (!file || !file->Open(path, FILEOPEN::WRITE))
        return false;

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
    if (!doc) return 0;
    Int32 mode = doc->GetMode();

    PieContextMode desired = PieContextMode::Global;
    if (mode == Mpolygons) desired = PieContextMode::Polygons;
    else if (mode == Medges) desired = PieContextMode::Edges;
    else if (mode == Mpoints) desired = PieContextMode::Points;
    else if (mode == Mobject || mode == Mmodel) desired = PieContextMode::Objects;

    for (Int32 i = 0; i < (Int32)m_profiles.GetCount(); ++i)
    {
        if (m_profiles[i].context == desired)
            return i;
    }
    return 0; // Fallback to first profile
}

PieProfile* PieProfileManager::GetActiveProfileForDoc(BaseDocument* doc)
{
    Int32 idx = GetActiveProfileIndex(doc);
    return GetProfile(idx);
}

} // namespace cinema
