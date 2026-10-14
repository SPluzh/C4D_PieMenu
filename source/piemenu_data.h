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

enum class ToolPresetMode
{
    None = 0,
    Snapshot,
    Asset
};

struct PieSlot
{
    Int32         commandId = NOTOK;
    maxon::String subMenuName; // Name of target submenu profile if this slot opens a submenu
    maxon::String customLabel;
    Vector        accentColor = Vector(0.25, 0.65, 1.0);
    Bool          enabled = true;
    Int32         shiftCommandId = NOTOK;
    maxon::String shiftCustomLabel;
    Int32         ctrlCommandId = NOTOK;
    maxon::String ctrlCustomLabel;
    Int32         altCommandId = NOTOK;
    maxon::String altCustomLabel;

    // Tool preset settings (Option 4: Hybrid)
    ToolPresetMode toolPresetMode = ToolPresetMode::None;
    maxon::String  toolPresetName;
    maxon::String  toolPresetAssetId;
    BaseContainer  toolSettings;

    // Shift preset settings
    ToolPresetMode shiftPresetMode = ToolPresetMode::None;
    maxon::String  shiftPresetName;
    maxon::String  shiftPresetAssetId;
    BaseContainer  shiftSettings;

    // Ctrl preset settings
    ToolPresetMode ctrlPresetMode = ToolPresetMode::None;
    maxon::String  ctrlPresetName;
    maxon::String  ctrlPresetAssetId;
    BaseContainer  ctrlSettings;

    // Alt preset settings
    ToolPresetMode altPresetMode = ToolPresetMode::None;
    maxon::String  altPresetName;
    maxon::String  altPresetAssetId;
    BaseContainer  altSettings;

    Bool IsSubMenu() const { return subMenuName.IsPopulated(); }
    Bool HasToolPreset() const { return toolPresetMode != ToolPresetMode::None; }
    Bool HasSnapshot() const { return !toolSettings.IsEmpty(); }

    Bool HasShiftPreset() const { return shiftPresetMode != ToolPresetMode::None; }
    Bool HasShiftSnapshot() const { return !shiftSettings.IsEmpty(); }

    Bool HasCtrlPreset() const { return ctrlPresetMode != ToolPresetMode::None; }
    Bool HasCtrlSnapshot() const { return !ctrlSettings.IsEmpty(); }

    Bool HasAltPreset() const { return altPresetMode != ToolPresetMode::None; }
    Bool HasAltSnapshot() const { return !altSettings.IsEmpty(); }

    Bool HasModifierPresets() const
    {
        return (shiftPresetMode != ToolPresetMode::None || ctrlPresetMode != ToolPresetMode::None || altPresetMode != ToolPresetMode::None);
    }
    Bool HasAnyPreset() const
    {
        return HasToolPreset() || HasModifierPresets();
    }

    void ClearToolPreset()
    {
        toolPresetMode = ToolPresetMode::None;
        toolPresetName = ""_s;
        toolPresetAssetId = ""_s;
        toolSettings.FlushAll();
    }

    void ClearShiftPreset()
    {
        shiftPresetMode = ToolPresetMode::None;
        shiftPresetName = ""_s;
        shiftPresetAssetId = ""_s;
        shiftSettings.FlushAll();
    }

    void ClearCtrlPreset()
    {
        ctrlPresetMode = ToolPresetMode::None;
        ctrlPresetName = ""_s;
        ctrlPresetAssetId = ""_s;
        ctrlSettings.FlushAll();
    }

    void ClearAltPreset()
    {
        altPresetMode = ToolPresetMode::None;
        altPresetName = ""_s;
        altPresetAssetId = ""_s;
        altSettings.FlushAll();
    }

    void ClearAllPresets()
    {
        ClearToolPreset();
        ClearShiftPreset();
        ClearCtrlPreset();
        ClearAltPreset();
    }

    void ApplyToolPreset(BaseDocument* doc, Int32 executedCmd, Bool shift = false, Bool ctrl = false, Bool alt = false) const;

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

    void ClearShiftAction()
    {
        shiftCommandId = NOTOK;
        shiftCustomLabel = ""_s;
        ClearShiftPreset();
    }

    void ClearCtrlAction()
    {
        ctrlCommandId = NOTOK;
        ctrlCustomLabel = ""_s;
        ClearCtrlPreset();
    }

    void ClearAltAction()
    {
        altCommandId = NOTOK;
        altCustomLabel = ""_s;
        ClearAltPreset();
    }

    maxon::String GetEffectiveShiftTitle() const
    {
        if (shiftCustomLabel.IsPopulated())
            return shiftCustomLabel;
        if (shiftCommandId > 0)
            return GetCommandName(shiftCommandId);
        return ""_s;
    }

    maxon::String GetEffectiveCtrlTitle() const
    {
        if (ctrlCustomLabel.IsPopulated())
            return ctrlCustomLabel;
        if (ctrlCommandId > 0)
            return GetCommandName(ctrlCommandId);
        return ""_s;
    }

    maxon::String GetEffectiveAltTitle() const
    {
        if (altCustomLabel.IsPopulated())
            return altCustomLabel;
        if (altCommandId > 0)
            return GetCommandName(altCommandId);
        return ""_s;
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
    Vector         bgColor = Vector(0.09, 0.10, 0.13);
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
