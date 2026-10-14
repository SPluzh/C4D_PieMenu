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
    maxon::String subMenuName; // Name of target submenu profile if this slot opens a submenu
    maxon::String customLabel;
    Vector        accentColor = Vector(0.25, 0.65, 1.0);
    Bool          enabled = true;
    Int32         shiftCommandId = NOTOK;
    Int32         ctrlCommandId = NOTOK;
    Int32         altCommandId = NOTOK;

    Bool IsSubMenu() const { return subMenuName.IsPopulated(); }
    maxon::String GetEffectiveTitle() const;
    Bool GetIconBitmap(IconData& outIcon) const;

    Int32 GetCommandForModifiers(Bool shift, Bool ctrl, Bool alt) const
    {
        if (shift && shiftCommandId > 0) return shiftCommandId;
        if (ctrl && ctrlCommandId > 0) return ctrlCommandId;
        if (alt && altCommandId > 0) return altCommandId;
        return commandId;
    }

    Bool HasModifierCommands() const
    {
        return (shiftCommandId > 0 || ctrlCommandId > 0 || altCommandId > 0);
    }

    maxon::String GetEffectiveTitleForModifiers(Bool shift, Bool ctrl, Bool alt) const;
    Bool GetIconBitmapForModifiers(Bool shift, Bool ctrl, Bool alt, IconData& outIcon) const;
};

enum class PieDisplayMode
{
    IconAndLabel = 0, // Both icon and label
    IconOnly     = 1, // Only icon (no label)
    LabelOnly    = 2  // Only label (no icon)
};

enum class PieContextMode
{
    Global = 0,
    Polygons = 1,
    Edges = 2,
    Points = 3,
    Objects = 4,
    UV = 5,
    Components = 6
};

maxon::String GetContextName(PieContextMode ctx);
maxon::String GetContextShortName(PieContextMode ctx);
PieContextMode GetContextModeForDoc(BaseDocument* doc);
Bool IsComponentContext(PieContextMode ctx);

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

static constexpr Int32 MIN_SLOT_COUNT = 2;
static constexpr Int32 MAX_SLOT_COUNT = 24;
static constexpr Int32 CENTER_SLOT_INDEX = 24;
static constexpr Int32 TOTAL_SLOT_COUNT = 25;

struct PieProfile
{
    maxon::String  name;
    PieContextMode context = PieContextMode::Global;
    Int32          slotCount = 8; // 2..24 radial slots
    Bool           showTitle = true;
    Bool           showItemName = true;
    PieSlot        slots[TOTAL_SLOT_COUNT]; // 0..23 radial, 24 is Center Hub (CENTER_SLOT_INDEX)
    PieHotkey      hotkey;
    Float          outerRadius = 112.0;
    Float          innerRadius = 36.0;
    Float          radius = 112.0;
    Float          deadzone = 26.0;
    Float          iconSize = 24.0;
    Float          opacity = 1.0;
    PieDisplayMode displayMode = PieDisplayMode::IconOnly;
    Bool           stayOpenOnRelease = false;
    Bool           stayOpenWithCenterCommand = false;

    Float GetSlotAngle(Int32 index) const;
    Int32 AngleToSlot(Float deg, Float dist, Float deadzone) const;

    Bool IsRadialSlot(Int32 index) const { return index >= 0 && index < slotCount; }
    Bool IsCenterSlot(Int32 index) const { return index == CENTER_SLOT_INDEX; }
    Bool IsValidSlot(Int32 index) const { return IsRadialSlot(index) || IsCenterSlot(index); }
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
    PieProfile* FindProfileByName(const maxon::String& name);
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
    Float m_outerRadius = 112.0;
    Float m_innerRadius = 36.0;
    Float m_radius = 112.0;
    Float m_deadzone = 26.0;
    Float m_iconSize = 24.0;
    Float m_opacity = 1.0;
    PieDisplayMode m_displayMode = PieDisplayMode::IconOnly;
    Bool           m_stayOpenOnRelease = false;
    Bool           m_stayOpenWithCenterCommand = false;

private:
    PieProfileManager();
    ~PieProfileManager() = default;

    maxon::BaseArray<PieProfile> m_profiles;
    Filename GetConfigPath() const;
};

} // namespace cinema

#endif // PIEMENU_DATA_H__
