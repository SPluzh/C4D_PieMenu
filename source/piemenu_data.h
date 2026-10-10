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
    Objects,
    UV
};

maxon::String GetContextName(PieContextMode ctx);
maxon::String GetContextShortName(PieContextMode ctx);
PieContextMode GetContextModeForDoc(BaseDocument* doc);

struct PieHotkey
{
    Bool  enabled = true;
    Bool  ctrl = true;
    Bool  shift = true;
    Bool  alt = false;
    Int32 vkCode = 0; // Virtual key code ('A'-'Z', '0'-'9', VK_TAB, etc.). 0 = none / modifiers only

    maxon::String GetDisplayString() const;
    Bool IsPhysicallyDown() const;
    Bool IsMatch(Bool ctrlDown, Bool shiftDown, Bool altDown, Int32 key) const;
};

struct PieProfile
{
    maxon::String  name;
    PieContextMode context = PieContextMode::Global;
    Int32          slotCount = 8; // 4 or 8
    Bool           showTitle = true;
    PieSlot        slots[9];      // 0..7 radial, 8 is Center
    PieHotkey      hotkey;

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

    // Import and Export settings
    Bool ExportSettingsToFile(const Filename& path, Int32 singleProfileIndex = NOTOK) const;
    Bool ImportSettingsFromFile(const Filename& path, Bool replaceExisting, Int32& outImportedCount);
    Bool InspectImportFile(const Filename& path, Int32& outProfileCount, maxon::String& outFirstName, Bool& outHasGlobalSettings) const;

    Int32 GetProfileCount() const;
    PieProfile* GetProfile(Int32 index);
    PieProfile* GetActiveProfileForDoc(BaseDocument* doc);
    Int32 GetActiveProfileIndex(BaseDocument* doc);

    Int32 AddProfile(const maxon::String& name = maxon::String(), PieContextMode context = PieContextMode::Global, Int32 slotCount = 8);
    Int32 DuplicateProfile(Int32 sourceIndex);
    Bool DeleteProfile(Int32 index);
    Bool RenameProfile(Int32 index, const maxon::String& newName);
    Bool SetProfileContext(Int32 index, PieContextMode context);

    PieProfile* FindTriggeredProfile(BaseDocument* doc) const;
    Int32 FindTriggeredProfileIndex(BaseDocument* doc) const;
    Bool IsProfileHotkeyHeld(const PieProfile* prof) const;

    Bool  IsTriggerHeld() const;

    // Global settings
    Bool  m_useCtrlShift = false; // Legacy (retained for binary file compatibility)
    Int32 m_triggerKey = 'V';     // Legacy (retained for binary file compatibility)
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
