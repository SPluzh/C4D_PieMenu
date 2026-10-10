#include "piemenu_overlay_win.h"
#include <cmath>
#include <string>
#include <vector>
#include "c4d_general.h"

#if defined(MAXON_TARGET_WINDOWS)
#define LONG WinLONG
#define ULONG WinULONG
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include "maxon/utilities/undef_win_macros.h"
#undef LONG
#undef ULONG
#pragma comment(lib, "gdiplus.lib")
#endif

namespace cinema
{

#if defined(MAXON_TARGET_WINDOWS)

using Gdiplus::REAL;

static const wchar_t* OVERLAY_WNDCLASS = L"C4D_PieMenu_Overlay_Class";
static const int OVERLAY_SIZE = 500;
static const UINT_PTR TIMER_ID = 1001;

static HHOOK g_keyboardHook = NULL;

static inline int LocalClampInt(int v, int minV, int maxV) { return (v < minV) ? minV : ((v > maxV) ? maxV : v); }

static std::wstring ToWString(const maxon::String& str)
{
    const String& c4dStr = MaxonConvert(str);
    Char* cstr = c4dStr.GetCStringCopy(STRINGENCODING::UTF8);
    if (!cstr) return L"";
    int req = MultiByteToWideChar(CP_UTF8, 0, cstr, -1, nullptr, 0);
    if (req <= 1) { DeleteMem(cstr); return L""; }
    std::vector<wchar_t> buf(req);
    MultiByteToWideChar(CP_UTF8, 0, cstr, -1, buf.data(), req);
    DeleteMem(cstr);
    return std::wstring(buf.data());
}

static Bool IsUserTypingText()
{
    HWND hFocus = GetFocus();
    if (!hFocus)
        return false;

    char className[64] = {};
    GetClassNameA(hFocus, className, sizeof(className));
    if (_stricmp(className, "Edit") == 0 ||
        _stricmp(className, "RichEdit") == 0 ||
        _stricmp(className, "RichEdit20W") == 0 ||
        strstr(className, "Edit") != nullptr)
    {
        return true;
    }

    LRESULT dlgCode = SendMessageA(hFocus, WM_GETDLGCODE, 0, 0);
    if (dlgCode & DLGC_WANTCHARS)
        return true;

    return false;
}

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* pkb = (KBDLLHOOKSTRUCT*)lParam;

        HWND fg = GetForegroundWindow();
        DWORD pid = 0;
        GetWindowThreadProcessId(fg, &pid);
        if (pid == GetCurrentProcessId())
        {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN ||
                wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
            {
                if (pkb->vkCode == VK_ESCAPE && PieMenuOverlay::Get().IsActive())
                {
                    PieMenuOverlay::Get().CancelAndClose();
                    return 1;
                }

                PieMenuOverlay::Get().CheckTrigger();
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

static LRESULT CALLBACK OverlayWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCCREATE)
    {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    PieMenuOverlay* overlay = (PieMenuOverlay*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
    if (overlay)
    {
        return (LRESULT)overlay->HandleMessage((void*)hwnd, msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

#endif // MAXON_TARGET_WINDOWS

PieMenuOverlay& PieMenuOverlay::Get()
{
    static PieMenuOverlay s_instance;
    return s_instance;
}

PieMenuOverlay::PieMenuOverlay()
{
}

PieMenuOverlay::~PieMenuOverlay()
{
    Shutdown();
}

Bool PieMenuOverlay::Init()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (m_initialized)
        return true;

    // 1. Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiInput;
    Gdiplus::GdiplusStartup((ULONG_PTR*)&m_gdiToken, &gdiInput, NULL);

    // 2. Register overlay window class
    HINSTANCE hInst = GetModuleHandleW(NULL);
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = OverlayWndProc;
    wcex.hInstance = hInst;
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.lpszClassName = OVERLAY_WNDCLASS;
    RegisterClassExW(&wcex);

    // 3. Create window
    HWND hwnd = CreateWindowExW(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        OVERLAY_WNDCLASS,
        L"C4D_PieMenu_Overlay",
        WS_POPUP,
        0, 0, OVERLAY_SIZE, OVERLAY_SIZE,
        NULL, NULL, hInst, this
    );

    if (!hwnd)
        return false;

    m_hwnd = (void*)hwnd;
    m_initialized = true;

    // 4. Install WH_KEYBOARD_LL hook
    if (!g_keyboardHook)
    {
        g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, hInst, 0);
    }

    return true;
#else
    return false;
#endif
}

void PieMenuOverlay::Shutdown()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (g_keyboardHook)
    {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = NULL;
    }

    if (m_hwnd)
    {
        DestroyWindow((HWND)m_hwnd);
        m_hwnd = nullptr;
    }

    UnregisterClassW(OVERLAY_WNDCLASS, GetModuleHandleW(NULL));

    if (m_gdiToken)
    {
        Gdiplus::GdiplusShutdown((ULONG_PTR)m_gdiToken);
        m_gdiToken = 0;
    }

    m_initialized = false;
    m_active = false;
#endif
}

Bool PieMenuOverlay::Open()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_hwnd || m_active)
        return false;

    PieProfileManager& mgr = PieProfileManager::Get();
    BaseDocument* doc = GetActiveDocument();
    m_activeProfile = mgr.GetActiveProfileForDoc(doc);
    if (!m_activeProfile)
        return false;

    POINT pt;
    GetCursorPos(&pt);
    m_originScreenX = pt.x;
    m_originScreenY = pt.y;
    m_currentScreenX = pt.x;
    m_currentScreenY = pt.y;
    m_hoveredSlot = NOTOK;

    int winX = m_originScreenX - OVERLAY_SIZE / 2;
    int winY = m_originScreenY - OVERLAY_SIZE / 2;

    SetWindowPos((HWND)m_hwnd, HWND_TOPMOST, winX, winY, OVERLAY_SIZE, OVERLAY_SIZE,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    SetCapture((HWND)m_hwnd);
    m_active = true;

    SetTimer((HWND)m_hwnd, TIMER_ID, 10, NULL);

    RenderOverlay();

    if (m_activeProfile)
    {
        StatusSetText(FormatString("Pie Menu [@]: Move cursor toward a slot to select"_s, m_activeProfile->name));
    }

    return true;
#else
    return false;
#endif
}

void PieMenuOverlay::CommitAndClose()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_active)
        return;

    KillTimer((HWND)m_hwnd, TIMER_ID);
    ReleaseCapture();
    ShowWindow((HWND)m_hwnd, SW_HIDE);
    m_active = false;
    StatusSetText(""_s);

    Int32 commandToCall = NOTOK;
    if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < 9)
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (slot.enabled && slot.commandId > 0)
        {
            commandToCall = slot.commandId;
        }
    }

    m_hoveredSlot = NOTOK;
    m_activeProfile = nullptr;

    if (commandToCall > 0)
    {
        CallCommand(commandToCall);
    }
#endif
}

void PieMenuOverlay::CancelAndClose()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_active)
        return;

    KillTimer((HWND)m_hwnd, TIMER_ID);
    ReleaseCapture();
    ShowWindow((HWND)m_hwnd, SW_HIDE);
    m_active = false;
    m_hoveredSlot = NOTOK;
    m_activeProfile = nullptr;
    StatusSetText(""_s);
#endif
}

void PieMenuOverlay::CheckTrigger()
{
#if defined(MAXON_TARGET_WINDOWS)
    HWND fg = GetForegroundWindow();
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid != GetCurrentProcessId())
        return;

    PieProfileManager& mgr = PieProfileManager::Get();
    Bool held = mgr.IsTriggerHeld();

    if (held)
    {
        if (!m_active)
        {
            if (!mgr.m_useCtrlShift && IsUserTypingText())
                return;

            Open();
        }
    }
    else
    {
        if (m_active)
        {
            CommitAndClose();
        }
    }
#endif
}

void PieMenuOverlay::OnMouseMove(Int32 screenX, Int32 screenY)
{
    if (!m_active)
        return;

    m_currentScreenX = screenX;
    m_currentScreenY = screenY;

    Float dx = Float(m_currentScreenX - m_originScreenX);
    Float dy = Float(m_currentScreenY - m_originScreenY);
    UpdateHover(dx, dy);
    RenderOverlay();
}

void PieMenuOverlay::UpdateHover(Float dx, Float dy)
{
    if (!m_activeProfile)
    {
        m_hoveredSlot = NOTOK;
        return;
    }

    PieProfileManager& mgr = PieProfileManager::Get();
    Float dist = std::sqrt(dx * dx + dy * dy);
    const Float deadzone = (mgr.m_deadzone > 0.0) ? mgr.m_deadzone : 26.0;

    if (dist < deadzone)
    {
        if (m_activeProfile->slots[8].enabled && m_activeProfile->slots[8].commandId > 0)
        {
            m_hoveredSlot = 8;
        }
        else
        {
            m_hoveredSlot = NOTOK;
        }
    }
    else
    {
        Float angleRad = std::atan2(dy, dx);
        Float deg = angleRad * (180.0 / PI);
        m_hoveredSlot = m_activeProfile->AngleToSlot(deg, dist, deadzone);
    }

    if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < 9)
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        StatusSetText(FormatString("Pie Menu [@]: @ | Release trigger to execute"_s, m_activeProfile->name, slot.GetEffectiveTitle()));
    }
    else
    {
        StatusSetText(FormatString("Pie Menu [@]: Move cursor toward a slot to select"_s, m_activeProfile ? m_activeProfile->name : "Radial"_s));
    }
}

Int64 PieMenuOverlay::HandleMessage(void* hwndVal, UInt32 msg, UInt64 wParam, Int64 lParam)
{
#if defined(MAXON_TARGET_WINDOWS)
    HWND hwnd = (HWND)hwndVal;
    switch (msg)
    {
        case WM_MOUSEMOVE:
        {
            if (m_active)
            {
                POINT pt;
                GetCursorPos(&pt);
                OnMouseMove(pt.x, pt.y);
            }
            return 0;
        }

        case WM_LBUTTONUP:
        case WM_LBUTTONDOWN:
        {
            if (m_active)
            {
                CommitAndClose();
            }
            return 0;
        }

        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        {
            if (m_active)
            {
                CancelAndClose();
            }
            return 0;
        }

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            if (wParam == VK_ESCAPE)
            {
                CancelAndClose();
                return 0;
            }
            break;
        }

        case WM_TIMER:
        {
            if (wParam == TIMER_ID && m_active)
            {
                if (!PieProfileManager::Get().IsTriggerHeld())
                {
                    CommitAndClose();
                    return 0;
                }

                POINT pt;
                GetCursorPos(&pt);
                if (pt.x != m_currentScreenX || pt.y != m_currentScreenY)
                {
                    OnMouseMove(pt.x, pt.y);
                }
            }
            return 0;
        }

        case WM_ACTIVATE:
        {
            if (LOWORD(wParam) == WA_INACTIVE && m_active)
            {
                HWND fg = GetForegroundWindow();
                DWORD pid = 0;
                GetWindowThreadProcessId(fg, &pid);
                if (pid != GetCurrentProcessId())
                {
                    CancelAndClose();
                }
            }
            return 0;
        }
    }

    return (Int64)DefWindowProcW(hwnd, msg, (WPARAM)wParam, (LPARAM)lParam);
#else
    return 0;
#endif
}

void PieMenuOverlay::RenderOverlay()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_hwnd || !m_active || !m_activeProfile)
        return;

    HWND hwnd = (HWND)m_hwnd;
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = OVERLAY_SIZE;
    bmi.bmiHeader.biHeight = -OVERLAY_SIZE; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hBitmap);

    {
        Gdiplus::Bitmap bmp(OVERLAY_SIZE, OVERLAY_SIZE, OVERLAY_SIZE * 4, PixelFormat32bppPARGB, (BYTE*)pBits);
        Gdiplus::Graphics g(&bmp);

        g.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        g.Clear(Gdiplus::Color(0, 0, 0, 0));

        PieProfileManager& mgr = PieProfileManager::Get();

        const float cx = (float)(OVERLAY_SIZE * 0.5f);
        const float cy = (float)(OVERLAY_SIZE * 0.5f);

        float radiusScale = (float)((mgr.m_radius > 60.0) ? (mgr.m_radius / 85.0) : 1.0);
        const float deadzoneRadius = (float)((mgr.m_deadzone > 0.0) ? mgr.m_deadzone : 26.0);
        const float baseInnerR = 36.0f * radiusScale;
        const float baseOuterR = 112.0f * radiusScale;
        const float hoverInnerR = 33.0f * radiusScale;
        const float hoverOuterR = 125.0f * radiusScale;

        Int32 count = m_activeProfile->slotCount;
        if (count != 4 && count != 8)
            count = 8;

        const float sliceDeg = 360.0f / (float)count;
        const float gapDeg = (count == 4) ? 2.2f : 1.8f;
        const float deg2rad = (float)(PI / 180.0);

        // 0. Soft radial ambient backdrop shadow
        {
            Gdiplus::GraphicsPath shadowPath;
            float shR = hoverOuterR + 14.0f;
            shadowPath.AddEllipse(cx - shR, cy - shR, shR * 2.0f, shR * 2.0f);
            Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(55, 0, 0, 0));
            g.FillPath(&shadowBrush, &shadowPath);
        }

        // 1. Draw sectors
        for (Int32 i = 0; i < count; ++i)
        {
            const PieSlot& slot = m_activeProfile->slots[i];
            Bool isHovered = (m_hoveredSlot == i);

            float rIn = isHovered ? hoverInnerR : baseInnerR;
            float rOut = isHovered ? hoverOuterR : baseOuterR;

            float startDeg = 270.0f - sliceDeg * 0.5f + (float)i * sliceDeg + gapDeg * 0.5f;
            float sweepDeg = sliceDeg - gapDeg;

            Gdiplus::GraphicsPath path;
            path.AddArc(cx - rOut, cy - rOut, rOut * 2.0f, rOut * 2.0f, startDeg, sweepDeg);
            path.AddArc(cx - rIn, cy - rIn, rIn * 2.0f, rIn * 2.0f, startDeg + sweepDeg, -sweepDeg);
            path.CloseFigure();

            // Background color
            Gdiplus::Color bgCol;
            if (!slot.enabled)
            {
                bgCol = Gdiplus::Color(170, 16, 17, 20);
            }
            else if (isHovered)
            {
                BYTE r = (BYTE)LocalClampInt((int)(35 + slot.accentColor.x * 65.0), 0, 255);
                BYTE gr = (BYTE)LocalClampInt((int)(40 + slot.accentColor.y * 65.0), 0, 255);
                BYTE b = (BYTE)LocalClampInt((int)(50 + slot.accentColor.z * 65.0), 0, 255);
                bgCol = Gdiplus::Color(245, r, gr, b);
            }
            else if (slot.commandId > 0)
            {
                BYTE r = (BYTE)LocalClampInt((int)(26 + slot.accentColor.x * 20.0), 0, 255);
                BYTE gr = (BYTE)LocalClampInt((int)(28 + slot.accentColor.y * 20.0), 0, 255);
                BYTE b = (BYTE)LocalClampInt((int)(36 + slot.accentColor.z * 20.0), 0, 255);
                bgCol = Gdiplus::Color(220, r, gr, b);
            }
            else
            {
                bgCol = Gdiplus::Color(205, 23, 26, 33);
            }

            Gdiplus::SolidBrush bgBrush(bgCol);
            g.FillPath(&bgBrush, &path);

            // Active glow inner strip
            if (isHovered && slot.enabled)
            {
                float stripW = 4.5f * radiusScale;
                Gdiplus::GraphicsPath stripPath;
                stripPath.AddArc(cx - (rIn + stripW), cy - (rIn + stripW), (rIn + stripW) * 2.0f, (rIn + stripW) * 2.0f, startDeg, sweepDeg);
                stripPath.AddArc(cx - rIn, cy - rIn, rIn * 2.0f, rIn * 2.0f, startDeg + sweepDeg, -sweepDeg);
                stripPath.CloseFigure();
                BYTE ar = (BYTE)(Clamp01(slot.accentColor.x) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot.accentColor.y) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot.accentColor.z) * 255.0);
                Gdiplus::SolidBrush stripBrush(Gdiplus::Color(255, ar, ag, ab));
                g.FillPath(&stripBrush, &stripPath);
            }
            else if (slot.enabled && slot.commandId > 0)
            {
                float stripW = 3.0f * radiusScale;
                Gdiplus::GraphicsPath stripPath;
                stripPath.AddArc(cx - (rIn + stripW), cy - (rIn + stripW), (rIn + stripW) * 2.0f, (rIn + stripW) * 2.0f, startDeg, sweepDeg);
                stripPath.AddArc(cx - rIn, cy - rIn, rIn * 2.0f, rIn * 2.0f, startDeg + sweepDeg, -sweepDeg);
                stripPath.CloseFigure();
                BYTE ar = (BYTE)(Clamp01(slot.accentColor.x * 0.6) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot.accentColor.y * 0.6) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot.accentColor.z * 0.6) * 255.0);
                Gdiplus::SolidBrush stripBrush(Gdiplus::Color(180, ar, ag, ab));
                g.FillPath(&stripBrush, &stripPath);
            }

            // Sector border
            Gdiplus::Color borderCol;
            float borderWidth = 1.0f;
            if (!slot.enabled)
            {
                borderCol = Gdiplus::Color(150, 42, 45, 50);
                borderWidth = 1.0f;
            }
            else if (isHovered)
            {
                BYTE ar = (BYTE)(Clamp01(slot.accentColor.x) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot.accentColor.y) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot.accentColor.z) * 255.0);
                borderCol = Gdiplus::Color(255, ar, ag, ab);
                borderWidth = 2.0f;
            }
            else if (slot.commandId > 0)
            {
                BYTE ar = (BYTE)(Clamp01(slot.accentColor.x * 0.75) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot.accentColor.y * 0.75) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot.accentColor.z * 0.75) * 255.0);
                borderCol = Gdiplus::Color(220, ar, ag, ab);
                borderWidth = 1.2f;
            }
            else
            {
                borderCol = Gdiplus::Color(180, 60, 68, 78);
                borderWidth = 1.0f;
            }
            Gdiplus::Pen borderPen(borderCol, borderWidth);
            g.DrawPath(&borderPen, &path);
        }

        // 2. Draw Labels inside segments
        Gdiplus::Font font(L"Segoe UI", 9.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        for (Int32 i = 0; i < count; ++i)
        {
            const PieSlot& slot = m_activeProfile->slots[i];
            Bool isHovered = (m_hoveredSlot == i);

            float rIn = isHovered ? hoverInnerR : baseInnerR;
            float rOut = isHovered ? hoverOuterR : baseOuterR;
            float rText = (rIn + rOut) * 0.5f;

            float midDeg = 270.0f + (float)i * sliceDeg;
            float midRad = midDeg * deg2rad;

            float tx = cx + std::cos(midRad) * rText;
            float ty = cy + std::sin(midRad) * rText;

            String label = slot.GetEffectiveTitle();
            std::wstring wLabel = ToWString(label);

            Gdiplus::Color textCol;
            if (!slot.enabled)
            {
                textCol = Gdiplus::Color(160, 100, 105, 115);
            }
            else if (isHovered)
            {
                textCol = Gdiplus::Color(255, 255, 255, 255);
            }
            else if (slot.commandId > 0)
            {
                BYTE ar = (BYTE)(Clamp01(slot.accentColor.x) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot.accentColor.y) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot.accentColor.z) * 255.0);
                textCol = Gdiplus::Color(255, ar, ag, ab);
            }
            else
            {
                textCol = Gdiplus::Color(220, 200, 210, 225);
            }

            // Drop shadow for text readability
            Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(180, 0, 0, 0));
            g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(tx + 1.0f, ty + 1.0f), &format, &shadowBrush);

            Gdiplus::SolidBrush textBrush(textCol);
            g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(tx, ty), &format, &textBrush);
        }

        // 3. Center Hub (Slot 8 / deadzone)
        const PieSlot& slot8 = m_activeProfile->slots[8];
        Bool centerConfigured = (slot8.enabled && slot8.commandId > 0);
        Bool centerHovered = (m_hoveredSlot == 8);

        Gdiplus::GraphicsPath hubPath;
        hubPath.AddEllipse(cx - deadzoneRadius, cy - deadzoneRadius, deadzoneRadius * 2.0f, deadzoneRadius * 2.0f);

        Gdiplus::Color hubBgCol;
        if (centerHovered)
        {
            BYTE r = (BYTE)LocalClampInt((int)(35 + slot8.accentColor.x * 65.0), 0, 255);
            BYTE gr = (BYTE)LocalClampInt((int)(40 + slot8.accentColor.y * 65.0), 0, 255);
            BYTE b = (BYTE)LocalClampInt((int)(50 + slot8.accentColor.z * 65.0), 0, 255);
            hubBgCol = Gdiplus::Color(245, r, gr, b);
        }
        else if (centerConfigured)
        {
            BYTE r = (BYTE)LocalClampInt((int)(26 + slot8.accentColor.x * 25.0), 0, 255);
            BYTE gr = (BYTE)LocalClampInt((int)(28 + slot8.accentColor.y * 25.0), 0, 255);
            BYTE b = (BYTE)LocalClampInt((int)(36 + slot8.accentColor.z * 25.0), 0, 255);
            hubBgCol = Gdiplus::Color(230, r, gr, b);
        }
        else
        {
            hubBgCol = Gdiplus::Color(220, 26, 28, 35);
        }

        Gdiplus::SolidBrush hubBgBrush(hubBgCol);
        g.FillPath(&hubBgBrush, &hubPath);

        Gdiplus::Color hubBorderCol;
        if (centerHovered)
        {
            BYTE ar = (BYTE)(Clamp01(slot8.accentColor.x) * 255.0);
            BYTE ag = (BYTE)(Clamp01(slot8.accentColor.y) * 255.0);
            BYTE ab = (BYTE)(Clamp01(slot8.accentColor.z) * 255.0);
            hubBorderCol = Gdiplus::Color(255, ar, ag, ab);
        }
        else if (m_hoveredSlot != NOTOK && m_hoveredSlot >= 0 && m_hoveredSlot < count)
        {
            const Vector& hCol = m_activeProfile->slots[m_hoveredSlot].accentColor * 0.8;
            hubBorderCol = Gdiplus::Color(220, (BYTE)(hCol.x * 255), (BYTE)(hCol.y * 255), (BYTE)(hCol.z * 255));
        }
        else if (centerConfigured)
        {
            const Vector& cCol = slot8.accentColor * 0.65;
            hubBorderCol = Gdiplus::Color(200, (BYTE)(cCol.x * 255), (BYTE)(cCol.y * 255), (BYTE)(cCol.z * 255));
        }
        else
        {
            hubBorderCol = Gdiplus::Color(180, 80, 88, 100);
        }

        Gdiplus::Pen hubPen(hubBorderCol, centerHovered ? 2.0f : 1.2f);
        g.DrawPath(&hubPen, &hubPath);

        if (centerConfigured)
        {
            std::wstring wCTitle = ToWString(slot8.GetEffectiveTitle());
            Gdiplus::Color cTextCol = centerHovered ? Gdiplus::Color(255, 255, 255, 255) :
                Gdiplus::Color(255, (BYTE)(slot8.accentColor.x * 255), (BYTE)(slot8.accentColor.y * 255), (BYTE)(slot8.accentColor.z * 255));
            Gdiplus::Font cFont(L"Segoe UI", 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush cTextBrush(cTextCol);
            g.DrawString(wCTitle.c_str(), -1, &cFont, Gdiplus::PointF(cx, cy), &format, &cTextBrush);
        }
        else
        {
            // Crosshair
            Gdiplus::Pen crosshairPen(Gdiplus::Color(200, 130, 140, 155), 1.0f);
            g.DrawLine(&crosshairPen, cx - 4.0f, cy, cx + 4.0f, cy);
            g.DrawLine(&crosshairPen, cx, cy - 4.0f, cx, cy + 4.0f);
        }

        // 4. Direction pointer line from center to cursor (when outside deadzone)
        float vdx = (float)(m_currentScreenX - m_originScreenX);
        float vdy = (float)(m_currentScreenY - m_originScreenY);
        float dist = std::sqrt(vdx * vdx + vdy * vdy);
        if (dist > deadzoneRadius)
        {
            Gdiplus::Color pointerCol;
            if (m_hoveredSlot != NOTOK && m_hoveredSlot >= 0 && m_hoveredSlot < count)
            {
                const Vector& c = m_activeProfile->slots[m_hoveredSlot].accentColor;
                pointerCol = Gdiplus::Color(240, (BYTE)(c.x * 255), (BYTE)(c.y * 255), (BYTE)(c.z * 255));
            }
            else if (centerHovered)
            {
                const Vector& c = slot8.accentColor;
                pointerCol = Gdiplus::Color(240, (BYTE)(c.x * 255), (BYTE)(c.y * 255), (BYTE)(c.z * 255));
            }
            else
            {
                pointerCol = Gdiplus::Color(180, 170, 175, 185);
            }

            float maxVisibleDist = (OVERLAY_SIZE * 0.5f) - 15.0f;
            float visualDist = (dist > maxVisibleDist) ? maxVisibleDist : dist;
            float pX = cx + (vdx / dist) * visualDist;
            float pY = cy + (vdy / dist) * visualDist;

            Gdiplus::Pen pointerPen(pointerCol, 1.5f);
            g.DrawLine(&pointerPen, cx, cy, pX, pY);

            Gdiplus::SolidBrush dotBrush(pointerCol);
            g.FillEllipse(&dotBrush, pX - 3.5f, pY - 3.5f, 7.0f, 7.0f);
        }

        // 5. Context badge at top: e.g. "[Polygons]"
        {
            std::wstring wBadge = L"[" + ToWString(m_activeProfile->name) + L"]";
            Gdiplus::Font bFont(L"Segoe UI", 8.5f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush bShadow(Gdiplus::Color(180, 0, 0, 0));
            Gdiplus::SolidBrush bBrush(Gdiplus::Color(220, 130, 180, 230));

            float bY = cy - hoverOuterR - 22.0f;
            g.DrawString(wBadge.c_str(), -1, &bFont, Gdiplus::PointF(cx + 1.0f, bY + 1.0f), &format, &bShadow);
            g.DrawString(wBadge.c_str(), -1, &bFont, Gdiplus::PointF(cx, bY), &format, &bBrush);
        }
    }

    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { OVERLAY_SIZE, OVERLAY_SIZE };
    POINT ptDst = { m_originScreenX - OVERLAY_SIZE / 2, m_originScreenY - OVERLAY_SIZE / 2 };
    BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    UpdateLayeredWindow(hwnd, hdcScreen, &ptDst, &sizeWnd, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
#endif
}

} // namespace cinema
