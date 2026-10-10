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
    void CommitAndClose();
    void CancelAndClose();

    void CheckTrigger();
    void OnMouseMove(Int32 screenX, Int32 screenY);

    Int64 HandleMessage(void* hwnd, UInt32 msg, UInt64 wParam, Int64 lParam);

private:
    PieMenuOverlay();
    ~PieMenuOverlay();

    void UpdateHover(Float dx, Float dy);
    void RenderOverlay();

    Bool        m_initialized = false;
    Bool        m_active = false;
    void*       m_hwnd = nullptr;
    UInt64      m_gdiToken = 0;

    Int32       m_originScreenX = 0;
    Int32       m_originScreenY = 0;
    Int32       m_currentScreenX = 0;
    Int32       m_currentScreenY = 0;
    Int32       m_hoveredSlot = NOTOK;
    PieProfile* m_activeProfile = nullptr;
};

} // namespace cinema

#endif // PIEMENU_OVERLAY_WIN_H__
