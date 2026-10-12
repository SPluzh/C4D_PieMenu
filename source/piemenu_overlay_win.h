#ifndef PIEMENU_OVERLAY_WIN_H__
#define PIEMENU_OVERLAY_WIN_H__

#include "c4d.h"
#include "piemenu_data.h"

namespace cinema
{

class PieMenuOverlay
{
public:
    static PieMenuOverlay& Get();

    Bool Init();
    void Shutdown();

    Bool IsActive() const { return m_active; }

    Bool Open();
    Bool OpenProfile(PieProfile* profile);
    void CommitAndClose(bool fromMouseClick = true);
    void CancelAndClose();
    void ClearIconCache();

    Bool NavigateToSubmenu(PieProfile* subProfile);
    Bool NavigateBack();
    Bool CanNavigateTo(const PieProfile* profile) const;

    static void SuppressModifierKeys();

    void CheckTrigger();
    void OnKeyboardHook(UInt32 vkCode, UInt32 msg);
    void SyncKeysFromAsync();
    Bool IsTriggerHeld() const;
    Bool IsHotkeyTriggered(const PieHotkey& hk) const;
    void OnMouseMove(Int32 screenX, Int32 screenY);

    Int64 HandleMessage(void* hwnd, UInt32 msg, UInt64 wParam, Int64 lParam);

private:
    PieMenuOverlay();
    ~PieMenuOverlay();

    Bool UpdateHover(Float dx, Float dy);
    void RenderOverlay();
    void ExecuteSlotCommand(const PieSlot& slot, Bool shiftDown, Bool ctrlDown, Bool altDown);

    Bool        m_initialized = false;
    Bool        m_active = false;
    void*       m_hwnd = nullptr;
    UInt64      m_gdiToken = 0;

    bool        m_ctrlDown = false;
    bool        m_shiftDown = false;
    bool        m_altDown = false;
    UInt32      m_currentKeyDown = 0;
    bool        m_triggerKeyDown = false;

    Int32       m_originScreenX = 0;
    Int32       m_originScreenY = 0;
    Int32       m_currentScreenX = 0;
    Int32       m_currentScreenY = 0;
    Int32       m_hoveredSlot = NOTOK;
    PieProfile* m_activeProfile = nullptr;

    maxon::BaseArray<PieProfile*> m_profileStack;
    UInt32      m_hoverStartTick = 0;
    Int32       m_lastHoveredSlot = NOTOK;
    bool        m_pinnedOpen = false;
    bool        m_mouseMovedOutsideDeadzone = false;

    bool        m_suppressTrigger = false;
    Int32       m_suppressedVk = 0;
    bool        m_suppressedCtrl = false;
    bool        m_suppressedShift = false;
    bool        m_suppressedAlt = false;
};

} // namespace cinema

#endif // PIEMENU_OVERLAY_WIN_H__
