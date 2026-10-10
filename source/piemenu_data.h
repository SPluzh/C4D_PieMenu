#ifndef PIEMENU_DATA_H__
#define PIEMENU_DATA_H__

#include "c4d.h"

namespace cinema
{

enum class PieSlotPos
{
    North = 0,     // Up (12:00)
    NorthEast,     // Up-Right (1:30)
    East,          // Right (3:00)
    SouthEast,     // Down-Right (4:30)
    South,         // Down (6:00)
    SouthWest,     // Down-Left (7:30)
    West,          // Left (9:00)
    NorthWest,     // Up-Left (10:30)
    Center         // Center Hub
};

struct PieSlot
{
    Int32         commandId = NOTOK;
    maxon::String customLabel;
    Vector        accentColor = Vector(0.25, 0.65, 1.0);
    Bool          enabled = true;

    maxon::String GetEffectiveTitle() const;
    Bool GetIconBitmap(IconData& outIcon) const;
};

enum class PieContextMode
{
    Global = 0,
    Polygons,
    Edges,
    Points,
    Objects
};

struct PieProfile
{
    maxon::String  name;
    PieContextMode context = PieContextMode::Global;
    Int32          slotCount = 8; // 4 or 8
    PieSlot        slots[9];      // 0..7 radial, 8 is Center

    Float GetSlotAngle(Int32 index) const;
    Int32 AngleToSlot(Float deg, Float dist, Float deadzone) const;
};

class PieProfileManager
{
public:
    static PieProfileManager& Get();

    void InitDefaults();
    Bool LoadFromFile();
    Bool SaveToFile();

    Int32 GetProfileCount() const;
    PieProfile* GetProfile(Int32 index);
    PieProfile* GetActiveProfileForDoc(BaseDocument* doc);
    Int32 GetActiveProfileIndex(BaseDocument* doc);

    // Global settings
    Bool  m_useCtrlShift = true;
    Int32 m_triggerKey = 'V';
    Float m_radius = 85.0;
    Float m_deadzone = 26.0;

private:
    PieProfileManager();
    ~PieProfileManager() = default;

    maxon::BaseArray<PieProfile> m_profiles;
    Filename GetConfigPath() const;
};

} // namespace cinema

#endif // PIEMENU_DATA_H__
