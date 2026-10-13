#include "piemenu_overlay_win.h"
#include "piemenu_editor_dialog.h"
#include <cmath>
#include <string>
#include <vector>
#include <unordered_map>
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

static std::unordered_map<Int32, Gdiplus::Bitmap*> s_gdiIconCache;

static void ClearGdiIconCache()
{
    for (auto& pair : s_gdiIconCache)
    {
        if (pair.second)
        {
            delete pair.second;
        }
    }
    s_gdiIconCache.clear();
}

static Gdiplus::Bitmap* GetOrCreateGdiIcon(Int32 commandId)
{
    if (commandId <= 0)
        return nullptr;

    auto it = s_gdiIconCache.find(commandId);
    if (it != s_gdiIconCache.end())
        return it->second;

    IconData icon;
    if (!GetIcon(commandId, &icon) || !icon.bmp)
    {
        s_gdiIconCache[commandId] = nullptr;
        return nullptr;
    }

    Int32 bmpW = icon.bmp->GetBw();
    Int32 bmpH = icon.bmp->GetBh();
    if (bmpW <= 0 || bmpH <= 0)
    {
        s_gdiIconCache[commandId] = nullptr;
        return nullptr;
    }

    Int32 w = icon.w;
    Int32 h = icon.h;
    if (w <= 0 || h <= 0)
    {
        w = bmpW;
        h = bmpH;
    }

    Int32 startX = icon.x;
    Int32 startY = icon.y;
    if (startX < 0) startX = 0;
    if (startY < 0) startY = 0;
    if (startX + w > bmpW) w = bmpW - startX;
    if (startY + h > bmpH) h = bmpH - startY;

    if (w <= 0 || h <= 0)
    {
        s_gdiIconCache[commandId] = nullptr;
        return nullptr;
    }

    Gdiplus::Bitmap* gdiBmp = new Gdiplus::Bitmap((INT)w, (INT)h, PixelFormat32bppPARGB);
    Gdiplus::Rect lockRect(0, 0, (INT)w, (INT)h);
    Gdiplus::BitmapData bmpData;

    if (gdiBmp->LockBits(&lockRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bmpData) != Gdiplus::Ok)
    {
        delete gdiBmp;
        s_gdiIconCache[commandId] = nullptr;
        return nullptr;
    }

    BaseBitmap* alpha = icon.bmp->GetInternalChannel();
    if (!alpha && icon.bmp->GetChannelCount() > 0)
    {
        alpha = icon.bmp->GetChannelNum(0);
    }

    BYTE* scan0 = (BYTE*)bmpData.Scan0;
    int stride = bmpData.Stride;

    for (Int32 row = 0; row < h; ++row)
    {
        DWORD* rowPixels = (DWORD*)(scan0 + row * stride);
        for (Int32 col = 0; col < w; ++col)
        {
            UInt16 r = 0, g = 0, b = 0;
            icon.bmp->GetPixel(startX + col, startY + row, &r, &g, &b);

            UInt16 a = 255;
            if (alpha)
            {
                icon.bmp->GetAlphaPixel(alpha, startX + col, startY + row, &a);
            }

            BYTE pa = (BYTE)(a > 255 ? 255 : a);
            BYTE pr = (BYTE)((r * pa + 127) / 255);
            BYTE pg = (BYTE)((g * pa + 127) / 255);
            BYTE pb = (BYTE)((b * pa + 127) / 255);

            rowPixels[col] = ((DWORD)pa << 24) | ((DWORD)pr << 16) | ((DWORD)pg << 8) | (DWORD)pb;
        }
    }

    gdiBmp->UnlockBits(&bmpData);
    s_gdiIconCache[commandId] = gdiBmp;
    return gdiBmp;
}

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

static bool IsEditControlClass(const char* className)
{
    if (!className || !className[0])
        return false;

    if (_stricmp(className, "Edit") == 0) return true;
    if (_stricmp(className, "RichEdit") == 0) return true;
    if (_stricmp(className, "RichEdit20W") == 0) return true;
    if (_stricmp(className, "RichEdit20A") == 0) return true;
    if (_stricmp(className, "RICHEDIT50W") == 0) return true;
    if (_stricmp(className, "RICHEDIT60W") == 0) return true;
    if (_stricmp(className, "MsftEdit_v4.0") == 0) return true;
    if (_stricmp(className, "Scintilla") == 0) return true;

    // Check if class name ends with "Edit" or "_Edit" (case-insensitive)
    // Matches e.g. "C4D_Edit" but NOT "Editor" or "EditorWindow"
    size_t len = strlen(className);
    if (len >= 4)
    {
        const char* suffix = className + len - 4;
        if (_stricmp(suffix, "Edit") == 0)
            return true;
    }

    return false;
}

static Bool IsUserTypingText()
{
    // 1. Direct check: Pie Menu Editor dialog fields
    if (PieMenuEditorDialog::IsAnyFieldActive())
        return true;

    HWND fg = GetForegroundWindow();
    if (!fg)
        return false;

    DWORD pid = 0;
    DWORD tid = GetWindowThreadProcessId(fg, &pid);
    if (pid != GetCurrentProcessId())
        return false;

    // 2. Query Win32 GUI Thread Info for foreground thread in Cinema 4D
    GUITHREADINFO gti = { sizeof(GUITHREADINFO) };
    if (GetGUIThreadInfo(tid, &gti))
    {
        // 2a. Caret check: if caret is active and blinking on the focused window
        if (gti.hwndCaret != NULL && gti.hwndFocus != NULL && gti.hwndCaret == gti.hwndFocus &&
            (gti.flags & GUI_CARETBLINKING) != 0)
        {
            return true;
        }

        // 2b. Focused window on the GUI thread
        if (gti.hwndFocus)
        {
            char className[128] = {};
            if (GetClassNameA(gti.hwndFocus, className, sizeof(className)) > 0)
            {
                if (IsEditControlClass(className))
                    return true;
            }
        }
    }

    // 3. Fallback: check calling thread's focus window
    HWND hFocus = GetFocus();
    if (hFocus)
    {
        char className[128] = {};
        if (GetClassNameA(hFocus, className, sizeof(className)) > 0)
        {
            if (IsEditControlClass(className))
                return true;
        }
    }

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
                // Hotkey assignment mode in Pie Menu configuration dialog
                if (PieMenuEditorDialog::IsCapturingHotkey() &&
                    (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
                {
                    if (pkb->vkCode == VK_ESCAPE)
                    {
                        PieMenuEditorDialog::s_activeDialog->CancelHotkeyCapture();
                        return 1;
                    }
                    if (pkb->vkCode != VK_CONTROL && pkb->vkCode != VK_LCONTROL && pkb->vkCode != VK_RCONTROL &&
                        pkb->vkCode != VK_SHIFT && pkb->vkCode != VK_LSHIFT && pkb->vkCode != VK_RSHIFT &&
                        pkb->vkCode != VK_MENU && pkb->vkCode != VK_LMENU && pkb->vkCode != VK_RMENU)
                    {
                        PieMenuEditorDialog::NotifyKeyCaptured((UInt32)pkb->vkCode);
                        return 1;
                    }
                }

                if (pkb->vkCode == VK_BACK && PieMenuOverlay::Get().IsActive())
                {
                    if (PieMenuOverlay::Get().NavigateBack())
                        return 1;
                }

                if (pkb->vkCode == VK_ESCAPE && PieMenuOverlay::Get().IsActive())
                {
                    PieMenuOverlay::Get().CancelAndClose();
                    return 1;
                }

                PieMenuOverlay::Get().OnKeyboardHook((UInt32)pkb->vkCode, (UInt32)wParam);
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

void PieMenuOverlay::ClearIconCache()
{
#if defined(MAXON_TARGET_WINDOWS)
    ClearGdiIconCache();
#endif
}

void PieMenuOverlay::Shutdown()
{
#if defined(MAXON_TARGET_WINDOWS)
    ClearGdiIconCache();

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
    m_suppressTrigger = false;
    m_suppressedVk = 0;
    m_suppressedCtrl = false;
    m_suppressedShift = false;
    m_suppressedAlt = false;
    m_mouseMovedOutsideDeadzone = false;
#endif
}

Bool PieMenuOverlay::Open()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    BaseDocument* doc = GetActiveDocument();
    PieProfile* prof = mgr.GetActiveProfileForDoc(doc);
    return OpenProfile(prof);
}

Bool PieMenuOverlay::OpenProfile(PieProfile* profile)
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_hwnd || m_active || !profile)
        return false;

    if (IsUserTypingText())
        return false;

    m_activeProfile = profile;
    m_profileStack.Reset();
    m_hoverStartTick = 0;
    m_lastHoveredSlot = NOTOK;
    m_pinnedOpen = false;
    m_mouseMovedOutsideDeadzone = false;

    // Record trigger combo to suppress re-trigger until physically released
    m_suppressedVk = profile->hotkey.vkCode;
    m_suppressedCtrl = profile->hotkey.ctrl;
    m_suppressedShift = profile->hotkey.shift;
    m_suppressedAlt = profile->hotkey.alt;
    m_suppressTrigger = false;

    POINT pt;
    GetCursorPos(&pt);
    m_originScreenX = pt.x;
    m_originScreenY = pt.y;
    m_currentScreenX = pt.x;
    m_currentScreenY = pt.y;
    m_hoveredSlot = NOTOK;

    int winX = m_originScreenX - OVERLAY_SIZE / 2;
    int winY = m_originScreenY - OVERLAY_SIZE / 2;

    m_active = true;

    // Render immediately so layered window has the first frame ready before display
    RenderOverlay();

    SetWindowPos((HWND)m_hwnd, HWND_TOPMOST, winX, winY, OVERLAY_SIZE, OVERLAY_SIZE,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    SetCapture((HWND)m_hwnd);

    SetTimer((HWND)m_hwnd, TIMER_ID, 10, NULL);

    if (m_activeProfile)
    {
        maxon::String ctxStr = GetContextShortName(m_activeProfile->context);
        StatusSetText(FormatString("Pie Menu [@ - @]: Move cursor toward a slot to select (Esc to cancel)"_s, m_activeProfile->name, ctxStr));
    }

    return true;
#else
    return false;
#endif
}

Bool PieMenuOverlay::CanNavigateTo(const PieProfile* profile) const
{
    if (!profile || profile == m_activeProfile)
        return false;

    for (const PieProfile* p : m_profileStack)
    {
        if (p == profile)
            return false;
    }

    return true;
}

Bool PieMenuOverlay::NavigateToSubmenu(PieProfile* subProfile)
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_active || !subProfile || !CanNavigateTo(subProfile))
        return false;

    m_profileStack.Append(m_activeProfile) iferr_ignore("Append");
    m_activeProfile = subProfile;
    m_hoveredSlot = NOTOK;
    m_lastHoveredSlot = NOTOK;
    m_hoverStartTick = 0;
    m_mouseMovedOutsideDeadzone = false;

    POINT pt;
    GetCursorPos(&pt);
    m_originScreenX = pt.x;
    m_originScreenY = pt.y;
    m_currentScreenX = pt.x;
    m_currentScreenY = pt.y;

    int winX = m_originScreenX - OVERLAY_SIZE / 2;
    int winY = m_originScreenY - OVERLAY_SIZE / 2;

    SetWindowPos((HWND)m_hwnd, HWND_TOPMOST, winX, winY, OVERLAY_SIZE, OVERLAY_SIZE,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    RenderOverlay();

    if (m_activeProfile)
    {
        maxon::String ctxStr = GetContextShortName(m_activeProfile->context);
        StatusSetText(FormatString("Pie Submenu [@ - @]: Select command (RMB / Backspace / Center to return)"_s, m_activeProfile->name, ctxStr));
    }

    return true;
#else
    return false;
#endif
}

Bool PieMenuOverlay::NavigateBack()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_active || m_profileStack.IsEmpty())
        return false;

    Int32 lastIdx = m_profileStack.GetCount() - 1;
    m_activeProfile = m_profileStack[lastIdx];
    m_profileStack.Erase(lastIdx) iferr_ignore("Erase");

    m_hoveredSlot = NOTOK;
    m_lastHoveredSlot = NOTOK;
    m_hoverStartTick = 0;
    m_mouseMovedOutsideDeadzone = false;

    POINT pt;
    GetCursorPos(&pt);
    m_originScreenX = pt.x;
    m_originScreenY = pt.y;
    m_currentScreenX = pt.x;
    m_currentScreenY = pt.y;

    int winX = m_originScreenX - OVERLAY_SIZE / 2;
    int winY = m_originScreenY - OVERLAY_SIZE / 2;

    SetWindowPos((HWND)m_hwnd, HWND_TOPMOST, winX, winY, OVERLAY_SIZE, OVERLAY_SIZE,
                 SWP_NOACTIVATE | SWP_SHOWWINDOW);

    RenderOverlay();

    if (m_activeProfile)
    {
        maxon::String ctxStr = GetContextShortName(m_activeProfile->context);
        StatusSetText(FormatString("Pie Menu [@ - @]: Move cursor toward a slot to select"_s, m_activeProfile->name, ctxStr));
    }

    return true;
#else
    return false;
#endif
}

void PieMenuOverlay::CommitAndClose(bool fromMouseClick)
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_active)
        return;

    const PieProfile* rootProfile = (!m_profileStack.IsEmpty() && m_profileStack[0]) ? m_profileStack[0] : m_activeProfile;
    const bool stayOpenOnRelease = rootProfile ? rootProfile->stayOpenOnRelease : (m_activeProfile ? m_activeProfile->stayOpenOnRelease : PieProfileManager::Get().m_stayOpenOnRelease);
    const bool stayOpenWithCenter = rootProfile ? rootProfile->stayOpenWithCenterCommand : (m_activeProfile ? m_activeProfile->stayOpenWithCenterCommand : PieProfileManager::Get().m_stayOpenWithCenterCommand);

    if (m_activeProfile && m_hoveredSlot >= 0 && (m_hoveredSlot < m_activeProfile->slotCount || m_hoveredSlot == CENTER_SLOT_INDEX))
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (slot.enabled && slot.IsSubMenu())
        {
            PieProfile* sub = PieProfileManager::Get().FindProfileByName(slot.subMenuName);
            if (sub && CanNavigateTo(sub))
            {
                if (fromMouseClick)
                {
                    NavigateToSubmenu(sub);
                    m_pinnedOpen = true;
                    return;
                }
                else if (stayOpenOnRelease)
                {
                    NavigateToSubmenu(sub);
                    m_pinnedOpen = true;
                    return;
                }
                // When releasing trigger and stayOpenOnRelease is false, do not pin open or navigate.
                // Fall through to close overlay.
            }
        }
        else if (m_hoveredSlot == CENTER_SLOT_INDEX && !m_profileStack.IsEmpty())
        {
            const bool hasCenterCmd = slot.enabled && (slot.commandId > 0 || slot.HasModifierCommands() || slot.IsSubMenu());
            if (!hasCenterCmd)
            {
                if (fromMouseClick)
                {
                    NavigateBack();
                    return;
                }
                else
                {
                    // Released trigger while in center hub of submenu
                    if (stayOpenOnRelease && !m_pinnedOpen)
                    {
                        m_pinnedOpen = true;
                        return;
                    }
                }
            }
            else
            {
                if (!fromMouseClick && stayOpenOnRelease && stayOpenWithCenter && !m_pinnedOpen)
                {
                    m_pinnedOpen = true;
                    return;
                }
            }
        }
        else if (m_hoveredSlot == CENTER_SLOT_INDEX && m_profileStack.IsEmpty() && !fromMouseClick)
        {
            const bool hasCenterCmd = slot.enabled && (slot.commandId > 0 || slot.HasModifierCommands() || slot.IsSubMenu());
            if (!hasCenterCmd)
            {
                // Released trigger in empty center hub at root
                if (stayOpenOnRelease && !m_pinnedOpen)
                {
                    m_pinnedOpen = true;
                    return;
                }
            }
            else
            {
                // Center hub has command assigned.
                // If stayOpenWithCenter is enabled along with stayOpenOnRelease, keep open on key release instead of executing center command.
                if (stayOpenOnRelease && stayOpenWithCenter && !m_pinnedOpen)
                {
                    m_pinnedOpen = true;
                    return;
                }
            }
        }
        else if (!fromMouseClick && stayOpenOnRelease && (!slot.enabled || (slot.commandId <= 0 && !slot.HasModifierCommands())))
        {
            // Released trigger over an unassigned slot with stayOpenOnRelease enabled -> pin open
            if (!m_pinnedOpen)
            {
                m_pinnedOpen = true;
                return;
            }
        }
    }
    else
    {
        if (!m_profileStack.IsEmpty())
        {
            // Check if inside center deadzone (Back button) when clicked with mouse
            Float dx = Float(m_currentScreenX - m_originScreenX);
            Float dy = Float(m_currentScreenY - m_originScreenY);
            Float dist = std::sqrt(dx * dx + dy * dy);
            PieProfileManager& mgr = PieProfileManager::Get();
            const Float deadzone = (mgr.m_deadzone > 0.0) ? mgr.m_deadzone : 26.0;
            if (fromMouseClick && dist < deadzone)
            {
                NavigateBack();
                return;
            }
        }

        if (!fromMouseClick)
        {
            // Released trigger while in center deadzone or gap
            if (stayOpenOnRelease && !m_pinnedOpen)
            {
                m_pinnedOpen = true;
                return;
            }
        }
    }

    PieSlot slotToCall;
    bool hasSlotToCall = false;

    if (m_activeProfile && m_hoveredSlot >= 0 && (m_hoveredSlot < m_activeProfile->slotCount || m_hoveredSlot == CENTER_SLOT_INDEX))
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (slot.enabled && !slot.IsSubMenu() && (slot.commandId > 0 || slot.HasModifierCommands()))
        {
            slotToCall = slot;
            hasSlotToCall = true;
        }
    }

    const PieHotkey* rootHk = (!m_profileStack.IsEmpty() && m_profileStack[0]) ? &m_profileStack[0]->hotkey : (m_activeProfile ? &m_activeProfile->hotkey : nullptr);

    bool callShift = m_shiftDown || ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
    bool callCtrl  = m_ctrlDown  || ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
    bool callAlt   = m_altDown   || ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) ||
                     ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);

    // If the trigger hotkey required a modifier to open the menu, that modifier was part of the trigger,
    // not an extra action modifier for the slot!
    if (rootHk && rootHk->enabled)
    {
        if (rootHk->shift) callShift = false;
        if (rootHk->ctrl)  callCtrl  = false;
        if (rootHk->alt)   callAlt   = false;
    }

    KillTimer((HWND)m_hwnd, TIMER_ID);
    ReleaseCapture();
    ShowWindow((HWND)m_hwnd, SW_HIDE);
    m_active = false;
    StatusSetText(""_s);

    m_ctrlDown = false;
    m_shiftDown = false;
    m_altDown = false;
    m_currentKeyDown = 0;
    m_triggerKeyDown = false;
    m_hoveredSlot = NOTOK;
    m_activeProfile = nullptr;
    m_profileStack.Reset();
    m_hoverStartTick = 0;
    m_lastHoveredSlot = NOTOK;
    m_pinnedOpen = false;
    m_mouseMovedOutsideDeadzone = false;

    // Suppress re-triggering until the trigger key/combination is physically released
    m_suppressTrigger = true;
    if (m_currentKeyDown != 0 && m_suppressedVk == 0)
    {
        m_suppressedVk = (Int32)m_currentKeyDown;
    }

    if (hasSlotToCall)
    {
        ExecuteSlotCommand(slotToCall, callShift, callCtrl, callAlt);
    }
#endif
}

void PieMenuOverlay::SuppressModifierKeys()
{
#if defined(MAXON_TARGET_WINDOWS)
    bool altDown   = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);
    bool shiftDown = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
    bool ctrlDown  = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);

    if (altDown || shiftDown || ctrlDown)
    {
        INPUT inputs[6] = {};
        int count = 0;
        if (altDown)
        {
            inputs[count].type = INPUT_KEYBOARD;
            inputs[count].ki.wVk = VK_MENU;
            inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
            count++;
        }
        if (shiftDown)
        {
            inputs[count].type = INPUT_KEYBOARD;
            inputs[count].ki.wVk = VK_SHIFT;
            inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
            count++;
        }
        if (ctrlDown)
        {
            inputs[count].type = INPUT_KEYBOARD;
            inputs[count].ki.wVk = VK_CONTROL;
            inputs[count].ki.dwFlags = KEYEVENTF_KEYUP;
            count++;
        }

        if (count > 0)
        {
            SendInput(count, inputs, sizeof(INPUT));
        }

        // Clear calling thread's keyboard state table
        BYTE ks[256];
        if (GetKeyboardState(ks))
        {
            if (altDown)   { ks[VK_MENU] = ks[VK_LMENU] = ks[VK_RMENU] = 0; }
            if (shiftDown) { ks[VK_SHIFT] = ks[VK_LSHIFT] = ks[VK_RSHIFT] = 0; }
            if (ctrlDown)  { ks[VK_CONTROL] = ks[VK_LCONTROL] = ks[VK_RCONTROL] = 0; }
            SetKeyboardState(ks);
        }

        // Dispatch keyup messages so Cinema 4D processes them immediately
        MSG msg;
        while (PeekMessage(&msg, NULL, WM_KEYFIRST, WM_KEYLAST, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
#endif
}

void PieMenuOverlay::ExecuteSlotCommand(const PieSlot& slot, Bool shiftDown, Bool ctrlDown, Bool altDown)
{
    // 1. Check if a custom command is configured for the active modifier
    Int32 customCmd = NOTOK;
    if (shiftDown && slot.shiftCommandId > 0)
        customCmd = slot.shiftCommandId;
    else if (ctrlDown && slot.ctrlCommandId > 0)
        customCmd = slot.ctrlCommandId;
    else if (altDown && slot.altCommandId > 0)
        customCmd = slot.altCommandId;

    if (customCmd > 0)
    {
        BaseDocument* doc = GetActiveDocument();
        maxon::BaseArray<BaseObject*> initialSelection;
        maxon::BaseArray<BaseObject*> initialParents;
        maxon::BaseArray<BaseObject*> initialPreds;
        if (doc)
        {
            AutoAlloc<AtomArray> activeAtomArray;
            if (activeAtomArray)
            {
                doc->GetActiveObjects(*activeAtomArray, GETACTIVEOBJECTFLAGS::CHILDREN);
                for (Int32 i = 0; i < activeAtomArray->GetCount(); ++i)
                {
                    C4DAtom* atom = activeAtomArray->GetIndex(i);
                    if (atom && atom->IsInstanceOf(Obase))
                    {
                        BaseObject* obj = static_cast<BaseObject*>(atom);
                        initialSelection.Append(obj) iferr_ignore("Append");
                        initialParents.Append(obj->GetUp()) iferr_ignore("Append");
                        initialPreds.Append(obj->GetPred()) iferr_ignore("Append");
                    }
                }
            }
        }

        BaseObject* firstObjBefore = doc ? doc->GetFirstObject() : nullptr;

        // Suppress modifiers in OS / C4D queue so custom command executes cleanly without modifier side-effects
        SuppressModifierKeys();

        CallCommand(customCmd);

        // Safeguard: if Cinema 4D natively parented or childed the initial selection, revert it
        if (doc && !initialSelection.IsEmpty())
        {
            BaseObject* newObj = nullptr;
            AutoAlloc<AtomArray> postAtomArray;
            if (postAtomArray)
            {
                doc->GetActiveObjects(*postAtomArray, GETACTIVEOBJECTFLAGS::CHILDREN);
                for (Int32 i = 0; i < postAtomArray->GetCount(); ++i)
                {
                    C4DAtom* atom = postAtomArray->GetIndex(i);
                    if (atom && atom->IsInstanceOf(Obase))
                    {
                        BaseObject* cand = static_cast<BaseObject*>(atom);
                        bool wasOld = false;
                        for (BaseObject* oldObj : initialSelection)
                        {
                            if (oldObj == cand) { wasOld = true; break; }
                        }
                        if (!wasOld) { newObj = cand; break; }
                    }
                }
            }
            if (!newObj)
            {
                BaseObject* activeObj = doc->GetActiveObject();
                if (activeObj)
                {
                    bool wasOld = false;
                    for (BaseObject* oldObj : initialSelection)
                    {
                        if (oldObj == activeObj) { wasOld = true; break; }
                    }
                    if (!wasOld) newObj = activeObj;
                }
            }
            if (!newObj && doc->GetFirstObject() && doc->GetFirstObject() != firstObjBefore)
            {
                newObj = doc->GetFirstObject();
            }

            if (newObj)
            {
                BaseObject* primary = initialSelection[0];
                if (primary->GetUp() == newObj)
                {
                    // Cinema 4D natively made newObj a parent of primary; restore original hierarchy
                    for (Int32 i = 0; i < (Int32)initialSelection.GetCount(); ++i)
                    {
                        BaseObject* oldObj = initialSelection[i];
                        Matrix oldMg = oldObj->GetMg();
                        oldObj->Remove();
                        BaseObject* origParent = (i < (Int32)initialParents.GetCount()) ? initialParents[i] : nullptr;
                        BaseObject* origPred = (i < (Int32)initialPreds.GetCount()) ? initialPreds[i] : nullptr;
                        if (origParent)
                        {
                            if (origPred) oldObj->InsertAfter(origPred);
                            else oldObj->InsertUnder(origParent);
                        }
                        else
                        {
                            if (origPred) oldObj->InsertAfter(origPred);
                            else doc->InsertObject(oldObj, nullptr, nullptr);
                        }
                        oldObj->SetMg(oldMg);
                    }
                    EventAdd();
                }
                else if (newObj->GetUp() == primary)
                {
                    // Cinema 4D natively made newObj a child of primary; move it to be a sibling
                    Matrix newMg = newObj->GetMg();
                    newObj->Remove();
                    BaseObject* parent = primary->GetUp();
                    if (parent)
                        newObj->InsertAfter(primary);
                    else
                        doc->InsertObject(newObj, nullptr, primary);
                    newObj->SetMg(newMg);
                    EventAdd();
                }
            }
        }

        return;
    }

    Int32 baseCmd = slot.commandId;
    if (baseCmd <= 0)
        return;

    // If no action modifiers were down (e.g. standard click, or modifiers were trigger hotkey only),
    // suppress any held trigger modifiers so C4D does not execute unwanted modifier behavior!
    if (!altDown && !shiftDown && !ctrlDown)
    {
        SuppressModifierKeys();
        CallCommand(baseCmd);
        return;
    }

    BaseDocument* doc = GetActiveDocument();
    if (!doc)
    {
        CallCommand(baseCmd);
        return;
    }

    // Collect currently selected objects in document before running the command
    maxon::BaseArray<BaseObject*> initialSelection;
    AutoAlloc<AtomArray> activeAtomArray;
    if (activeAtomArray)
    {
        doc->GetActiveObjects(*activeAtomArray, GETACTIVEOBJECTFLAGS::CHILDREN);
        for (Int32 i = 0; i < activeAtomArray->GetCount(); ++i)
        {
            C4DAtom* atom = activeAtomArray->GetIndex(i);
            if (atom && atom->IsInstanceOf(Obase))
            {
                initialSelection.Append(static_cast<BaseObject*>(atom)) iferr_ignore("Append");
            }
        }
    }

    BaseObject* firstObjBefore = doc->GetFirstObject();

    // Execute the command
    CallCommand(baseCmd);

    // If no modifiers were down, standard execution is finished
    if (!altDown && !shiftDown && !ctrlDown)
        return;

    // Check what objects are selected after command execution to find newly created object(s)
    maxon::BaseArray<BaseObject*> newObjects;
    AutoAlloc<AtomArray> postAtomArray;
    if (postAtomArray)
    {
        doc->GetActiveObjects(*postAtomArray, GETACTIVEOBJECTFLAGS::CHILDREN);
        for (Int32 i = 0; i < postAtomArray->GetCount(); ++i)
        {
            C4DAtom* atom = postAtomArray->GetIndex(i);
            if (atom && atom->IsInstanceOf(Obase))
            {
                BaseObject* cand = static_cast<BaseObject*>(atom);
                bool wasPreviouslySelected = false;
                for (BaseObject* oldObj : initialSelection)
                {
                    if (oldObj == cand)
                    {
                        wasPreviouslySelected = true;
                        break;
                    }
                }
                if (!wasPreviouslySelected)
                {
                    newObjects.Append(cand) iferr_ignore("Append");
                }
            }
        }
    }

    BaseObject* newObj = newObjects.IsEmpty() ? nullptr : newObjects[0];
    if (!newObj)
    {
        BaseObject* activeObj = doc->GetActiveObject();
        if (activeObj)
        {
            bool wasPreviouslySelected = false;
            for (BaseObject* oldObj : initialSelection)
            {
                if (oldObj == activeObj)
                {
                    wasPreviouslySelected = true;
                    break;
                }
            }
            if (!wasPreviouslySelected)
                newObj = activeObj;
        }
    }
    if (!newObj && doc->GetFirstObject() && doc->GetFirstObject() != firstObjBefore)
    {
        newObj = doc->GetFirstObject();
    }

    // CASE 1: Alt modifier (Make Parent of selected objects)
    if (altDown && newObj && !initialSelection.IsEmpty())
    {
        BaseObject* primary = initialSelection[0];
        // If Cinema 4D already parented the selection natively:
        if (primary->GetUp() == newObj)
        {
            EventAdd();
            return;
        }

        doc->StartUndo();
        doc->AddUndo(UNDOTYPE::CHANGE, newObj);

        Matrix targetMg = primary->GetMg();
        newObj->SetMg(targetMg);

        BaseObject* parent = primary->GetUp();
        if (parent)
        {
            newObj->Remove();
            newObj->InsertUnder(parent);
        }
        else
        {
            newObj->Remove();
            newObj->InsertBefore(primary);
        }

        for (BaseObject* oldObj : initialSelection)
        {
            Matrix oldMg = oldObj->GetMg();
            doc->AddUndo(UNDOTYPE::CHANGE, oldObj);
            oldObj->Remove();
            oldObj->InsertUnderLast(newObj);
            oldObj->SetMg(oldMg); // maintain world transform
        }

        doc->AddUndo(UNDOTYPE::NEWOBJ, newObj);
        doc->EndUndo();
        EventAdd();
        return;
    }

    // CASE 2: Shift modifier
    if (shiftDown)
    {
        if (newObj && !initialSelection.IsEmpty())
        {
            BaseObject* primary = initialSelection[0];
            if (newObj->GetUp() == primary)
            {
                EventAdd();
                return;
            }

            // Shift + Object creation -> Make Child of first selected object
            doc->StartUndo();
            doc->AddUndo(UNDOTYPE::CHANGE, newObj);

            newObj->Remove();
            newObj->InsertUnderLast(primary);
            newObj->SetMl(Matrix()); // Identity local matrix to align with parent

            doc->AddUndo(UNDOTYPE::NEWOBJ, newObj);
            doc->EndUndo();
            EventAdd();
            return;
        }
        else if (!newObj)
        {
            // Shift + Tool/Modeling command -> Open Command Option dialog
            BaseContainer msg(COREMSG_CINEMA_EXECUTEEDITORCOMMAND);
            msg.SetInt32(COREMSG_CINEMA_EXECUTEEDITORCOMMAND, baseCmd);
            msg.SetInt32(COREMSG_CINEMA_EXECUTESUBID, 0);
            msg.SetInt32(COREMSG_CINEMA_EXECUTEOPTIONMODE, true);
            SendCoreMessage(COREMSG_CINEMA, msg, 0);
            return;
        }
    }

    // CASE 3: Ctrl modifier (Place at world origin)
    if (ctrlDown && newObj)
    {
        doc->StartUndo();
        doc->AddUndo(UNDOTYPE::CHANGE, newObj);
        newObj->SetMg(Matrix()); // Origin (0,0,0)
        doc->EndUndo();
        EventAdd();
        return;
    }
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
    m_ctrlDown = false;
    m_shiftDown = false;
    m_altDown = false;
    m_currentKeyDown = 0;
    m_triggerKeyDown = false;
    m_hoveredSlot = NOTOK;
    m_activeProfile = nullptr;
    m_profileStack.Reset();
    m_hoverStartTick = 0;
    m_lastHoveredSlot = NOTOK;
    m_pinnedOpen = false;
    m_mouseMovedOutsideDeadzone = false;

    // Suppress re-triggering until the trigger key/combination is physically released
    m_suppressTrigger = true;
    if (m_currentKeyDown != 0 && m_suppressedVk == 0)
    {
        m_suppressedVk = (Int32)m_currentKeyDown;
    }
    StatusSetText(""_s);
#endif
}

void PieMenuOverlay::SyncKeysFromAsync()
{
#if defined(MAXON_TARGET_WINDOWS)
    m_ctrlDown = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
    m_shiftDown = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) ||
                  ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) ||
                  ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
    m_altDown = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) ||
                ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) ||
                ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);

    if (m_currentKeyDown != 0)
    {
        if ((GetAsyncKeyState(m_currentKeyDown) & 0x8000) == 0)
            m_currentKeyDown = 0;
    }
#endif
}

Bool PieMenuOverlay::IsHotkeyTriggered(const PieHotkey& hk) const
{
    if (!hk.enabled)
        return false;

#if defined(MAXON_TARGET_WINDOWS)
    bool cDown = m_ctrlDown || ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
    bool sDown = m_shiftDown || ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
    bool aDown = m_altDown || ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);

    if (hk.ctrl != cDown)
        return false;
    if (hk.shift != sDown)
        return false;
    if (hk.alt != aDown)
        return false;

    if (hk.vkCode == 0)
    {
        // Modifiers-only combo: requires at least one modifier
        return (hk.ctrl || hk.shift || hk.alt);
    }

    Int32 vk = hk.vkCode;
    if (vk >= 'a' && vk <= 'z')
        vk = toupper(vk);

    bool kDown = (m_currentKeyDown == (UInt32)vk) || ((GetAsyncKeyState(vk) & 0x8000) != 0);
    return kDown;
#else
    return false;
#endif
}

Bool PieMenuOverlay::IsTriggerHeld() const
{
    const PieHotkey* hk = nullptr;
    if (!m_profileStack.IsEmpty() && m_profileStack[0])
    {
        hk = &m_profileStack[0]->hotkey;
    }
    else if (m_activeProfile)
    {
        hk = &m_activeProfile->hotkey;
    }

    if (!hk || !hk->enabled)
        return false;

#if defined(MAXON_TARGET_WINDOWS)
    bool cDown = m_ctrlDown || ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
    bool sDown = m_shiftDown || ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
    bool aDown = m_altDown || ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) ||
                 ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);

    // If the trigger hotkey required Ctrl/Shift/Alt, they must still be held.
    // If not required by the trigger hotkey, pressing extra modifier keys is ALLOWED (they act as slot modifiers)!
    if (hk->ctrl && !cDown)
        return false;
    if (hk->shift && !sDown)
        return false;
    if (hk->alt && !aDown)
        return false;

    if (hk->vkCode == 0)
    {
        return (hk->ctrl || hk->shift || hk->alt);
    }

    Int32 vk = hk->vkCode;
    if (vk >= 'a' && vk <= 'z')
        vk = toupper(vk);

    bool kDown = (m_currentKeyDown == (UInt32)vk) || ((GetAsyncKeyState(vk) & 0x8000) != 0);
    return kDown;
#else
    return false;
#endif
}

void PieMenuOverlay::OnKeyboardHook(UInt32 vkCode, UInt32 msg)
{
#if defined(MAXON_TARGET_WINDOWS)
    bool isDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);
    bool isMod = false;

    if (vkCode == VK_CONTROL || vkCode == VK_LCONTROL || vkCode == VK_RCONTROL)
    {
        isMod = true;
        m_ctrlDown = isDown;
        if (isDown && !m_shiftDown && (GetAsyncKeyState(VK_SHIFT) & 0x8000))
        {
            m_shiftDown = true;
        }
        if (isDown && !m_altDown && (GetAsyncKeyState(VK_MENU) & 0x8000))
        {
            m_altDown = true;
        }
    }
    else if (vkCode == VK_SHIFT || vkCode == VK_LSHIFT || vkCode == VK_RSHIFT)
    {
        isMod = true;
        m_shiftDown = isDown;
        if (isDown && !m_ctrlDown && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
        {
            m_ctrlDown = true;
        }
        if (isDown && !m_altDown && (GetAsyncKeyState(VK_MENU) & 0x8000))
        {
            m_altDown = true;
        }
    }
    else if (vkCode == VK_MENU || vkCode == VK_LMENU || vkCode == VK_RMENU)
    {
        isMod = true;
        m_altDown = isDown;
        if (isDown && !m_ctrlDown && (GetAsyncKeyState(VK_CONTROL) & 0x8000))
        {
            m_ctrlDown = true;
        }
        if (isDown && !m_shiftDown && (GetAsyncKeyState(VK_SHIFT) & 0x8000))
        {
            m_shiftDown = true;
        }
    }
    else
    {
        if (isDown)
            m_currentKeyDown = vkCode;
        else if (m_currentKeyDown == vkCode)
            m_currentKeyDown = 0;
    }

    if (m_active && isMod)
    {
        RenderOverlay();
    }

    CheckTrigger();
#endif
}

void PieMenuOverlay::CheckTrigger()
{
#if defined(MAXON_TARGET_WINDOWS)
    HWND fg = GetForegroundWindow();
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid != GetCurrentProcessId())
    {
        if (m_active)
            CancelAndClose();
        m_ctrlDown = false;
        m_shiftDown = false;
        m_altDown = false;
        m_currentKeyDown = 0;
        m_triggerKeyDown = false;
        return;
    }

    if (m_active)
    {
        if (!m_pinnedOpen && !IsTriggerHeld())
        {
            CommitAndClose(false);
        }
    }
    else
    {
        if (m_suppressTrigger)
        {
            // Check if the previous trigger is still physically down
            bool stillHeld = false;
            if (m_suppressedVk != 0)
            {
                Int32 vk = m_suppressedVk;
                if (vk >= 'a' && vk <= 'z') vk = toupper(vk);
                if (((GetAsyncKeyState(vk) & 0x8000) != 0) || (m_currentKeyDown == (UInt32)vk))
                {
                    stillHeld = true;
                }
            }
            else if (m_suppressedCtrl || m_suppressedShift || m_suppressedAlt)
            {
                bool cDown = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
                bool sDown = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
                bool aDown = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);
                if ((m_suppressedCtrl && cDown) || (m_suppressedShift && sDown) || (m_suppressedAlt && aDown))
                {
                    stillHeld = true;
                }
            }

            if (stillHeld)
            {
                // Trigger key is still being held down since previous menu execution — do not re-open!
                return;
            }

            // Key has been physically released! Clear suppression
            m_suppressTrigger = false;
            m_suppressedVk = 0;
            m_suppressedCtrl = false;
            m_suppressedShift = false;
            m_suppressedAlt = false;
        }

        PieProfileManager& mgr = PieProfileManager::Get();
        BaseDocument* doc = GetActiveDocument();
        PieContextMode desired = GetContextModeForDoc(doc);

        // Find profiles matching current keys
        PieProfile* triggered = nullptr;
        maxon::BaseArray<PieProfile*> matched;
        for (Int32 i = 0; i < mgr.GetProfileCount(); ++i)
        {
            PieProfile* p = mgr.GetProfile(i);
            if (p && IsHotkeyTriggered(p->hotkey))
            {
                matched.Append(p) iferr_ignore("Append");
            }
        }

        if (!matched.IsEmpty())
        {
            // 1. Exact context match (e.g. Polygons mode while in Polygons)
            if (desired != PieContextMode::Global)
            {
                for (PieProfile* p : matched)
                {
                    if (p->context == desired)
                    {
                        triggered = p;
                        break;
                    }
                }
            }

            // 2. Components context match (Points, Edges, Polygons)
            if (!triggered && IsComponentContext(desired))
            {
                for (PieProfile* p : matched)
                {
                    if (p->context == PieContextMode::Components)
                    {
                        triggered = p;
                        break;
                    }
                }
            }

            // 3. Global context fallback
            if (!triggered)
            {
                for (PieProfile* p : matched)
                {
                    if (p->context == PieContextMode::Global)
                    {
                        triggered = p;
                        break;
                    }
                }
            }
        }

        if (triggered)
        {
            if (IsUserTypingText())
                return;

            OpenProfile(triggered);
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
    if (UpdateHover(dx, dy))
        return;

    RenderOverlay();
}

Bool PieMenuOverlay::UpdateHover(Float dx, Float dy)
{
    if (!m_activeProfile)
    {
        m_hoveredSlot = NOTOK;
        return false;
    }

    Float dist = std::sqrt(dx * dx + dy * dy);
    const Float deadzone = (m_activeProfile->deadzone > 0.0) ? m_activeProfile->deadzone : 26.0;

    if (dist >= deadzone)
    {
        m_mouseMovedOutsideDeadzone = true;
    }

    if (dist < deadzone)
    {
        if (m_activeProfile->slots[CENTER_SLOT_INDEX].enabled && (m_activeProfile->slots[CENTER_SLOT_INDEX].commandId > 0 || m_activeProfile->slots[CENTER_SLOT_INDEX].IsSubMenu() || m_activeProfile->slots[CENTER_SLOT_INDEX].HasModifierCommands()))
        {
            m_hoveredSlot = CENTER_SLOT_INDEX;
        }
        else if (!m_profileStack.IsEmpty())
        {
            m_hoveredSlot = CENTER_SLOT_INDEX;
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
        Int32 s = m_activeProfile->AngleToSlot(deg, dist, deadzone);
        Int32 count = m_activeProfile->slotCount;
        if (count < MIN_SLOT_COUNT) count = MIN_SLOT_COUNT;
        if (count > MAX_SLOT_COUNT) count = MAX_SLOT_COUNT;
        if (s >= 0 && s < count && !m_activeProfile->slots[s].enabled)
        {
            s = NOTOK;
        }
        m_hoveredSlot = s;
    }

    if (m_hoveredSlot != m_lastHoveredSlot)
    {
        m_lastHoveredSlot = m_hoveredSlot;
        m_hoverStartTick = (UInt32)GetTickCount();

        // Immediately expand submenu on hover without delay
        if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < m_activeProfile->slotCount)
        {
            const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
            if (slot.enabled && slot.IsSubMenu())
            {
                PieProfile* sub = PieProfileManager::Get().FindProfileByName(slot.subMenuName);
                if (sub && CanNavigateTo(sub))
                {
                    NavigateToSubmenu(sub);
                    return true;
                }
            }
        }
    }

    if (m_activeProfile && m_hoveredSlot >= 0 && (m_hoveredSlot < m_activeProfile->slotCount || m_hoveredSlot == CENTER_SLOT_INDEX))
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (m_hoveredSlot == CENTER_SLOT_INDEX && !m_profileStack.IsEmpty() && (!slot.enabled || (slot.commandId <= 0 && !slot.IsSubMenu())))
        {
            StatusSetText("Pie Menu: [Back to parent menu] (Click or release to go back)"_s);
        }
        else if (slot.IsSubMenu())
        {
            StatusSetText(FormatString("Pie Menu [@]: Submenu > @ | Hover or click to open"_s, m_activeProfile->name, slot.GetEffectiveTitle()));
        }
        else
        {
            const PieHotkey* hk = (!m_profileStack.IsEmpty() && m_profileStack[0]) ? &m_profileStack[0]->hotkey : (m_activeProfile ? &m_activeProfile->hotkey : nullptr);
            bool actShift = m_shiftDown;
            bool actCtrl  = m_ctrlDown;
            bool actAlt   = m_altDown;
            if (hk && hk->enabled)
            {
                if (hk->shift) actShift = false;
                if (hk->ctrl)  actCtrl  = false;
                if (hk->alt)   actAlt   = false;
            }

            String effTitle = slot.GetEffectiveTitleForModifiers(actShift, actCtrl, actAlt);
            String modHint = ""_s;
            if (actAlt && slot.altCommandId <= 0) modHint = " [ALT: Parent]"_s;
            else if (actShift && slot.shiftCommandId <= 0) modHint = " [SHIFT: Child / Options]"_s;
            else if (actCtrl && slot.ctrlCommandId <= 0) modHint = " [CTRL: Origin]"_s;

            StatusSetText(FormatString("Pie Menu [@]: @@ | Release trigger or click to execute"_s, m_activeProfile->name, effTitle, modHint));
        }
    }
    else
    {
        StatusSetText(FormatString("Pie Menu [@]: Move cursor toward a slot to select"_s, m_activeProfile ? m_activeProfile->name : "Radial"_s));
    }
    return false;
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

        case WM_LBUTTONDOWN:
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
        {
            if (m_active)
            {
                POINT pt;
                GetCursorPos(&pt);
                OnMouseMove(pt.x, pt.y);
                CommitAndClose(true);
            }
            return 0;
        }

        case WM_RBUTTONUP:
        case WM_RBUTTONDOWN:
        {
            if (m_active)
            {
                if (!m_profileStack.IsEmpty())
                {
                    NavigateBack();
                }
                else
                {
                    CancelAndClose();
                }
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
                SyncKeysFromAsync();

                if (!m_pinnedOpen && !IsTriggerHeld())
                {
                    CommitAndClose(false);
                    return 0;
                }

                // Check auto-expand submenu on hover (immediate)
                if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < m_activeProfile->slotCount)
                {
                    const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
                    if (slot.enabled && slot.IsSubMenu())
                    {
                        PieProfile* sub = PieProfileManager::Get().FindProfileByName(slot.subMenuName);
                        if (sub && CanNavigateTo(sub))
                        {
                            NavigateToSubmenu(sub);
                            return 0;
                        }
                    }
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

        const float deadzoneRadius = (float)((m_activeProfile->deadzone > 0.0) ? m_activeProfile->deadzone : 26.0);
        const float baseInnerR = (float)m_activeProfile->innerRadius;
        const float baseOuterR = (float)m_activeProfile->outerRadius;
        const float hoverInnerR = (baseInnerR > 3.0f) ? (baseInnerR - 3.0f) : baseInnerR;
        const float hoverOuterR = baseOuterR + 13.0f;

        Int32 count = m_activeProfile->slotCount;
        if (count < MIN_SLOT_COUNT) count = MIN_SLOT_COUNT;
        if (count > MAX_SLOT_COUNT) count = MAX_SLOT_COUNT;

        const float sliceDeg = 360.0f / (float)count;
        const float gapPx = (count <= 4) ? 3.0f : ((count <= 8) ? 2.5f : ((count <= 14) ? 2.0f : 1.6f));
        const float halfGapDist = gapPx * 0.5f;
        const float deg2rad = (float)(PI / 180.0);
        const float rad2deg = (float)(180.0 / PI);

        auto GetSectorAngles = [sliceDeg, halfGapDist, rad2deg](float r, float div0, float& outStart, float& outSweep)
        {
            float dAng = 0.0f;
            if (r > halfGapDist && r > 0.1f)
            {
                float ratio = halfGapDist / r;
                if (ratio > 0.999f) ratio = 0.999f;
                dAng = std::asinf(ratio) * rad2deg;
                if (dAng > sliceDeg * 0.48f)
                    dAng = sliceDeg * 0.48f;
            }
            outStart = div0 + dAng;
            outSweep = sliceDeg - 2.0f * dAng;
        };

        // 0. Soft radial ambient backdrop shadow behind visible slots and center hub
        {
            Gdiplus::GraphicsPath shadowPath;
            for (Int32 i = 0; i < count; ++i)
            {
                if (!m_activeProfile->slots[i].enabled)
                    continue;

                float rIn = (m_hoveredSlot == i) ? hoverInnerR : baseInnerR;
                float rOut = (m_hoveredSlot == i) ? hoverOuterR : baseOuterR;
                float shIn = rIn - 4.0f;
                if (shIn < 0.0f)
                    shIn = 0.0f;
                float shOut = rOut + 8.0f;

                float divDeg0 = 270.0f - sliceDeg * 0.5f + (float)i * sliceDeg;
                float startDegShOut, sweepDegShOut;
                float startDegShIn, sweepDegShIn;
                GetSectorAngles(shOut, divDeg0, startDegShOut, sweepDegShOut);
                GetSectorAngles(shIn,  divDeg0, startDegShIn,  sweepDegShIn);

                shadowPath.AddArc(cx - shOut, cy - shOut, shOut * 2.0f, shOut * 2.0f, startDegShOut, sweepDegShOut);
                shadowPath.AddArc(cx - shIn, cy - shIn, shIn * 2.0f, shIn * 2.0f, startDegShIn + sweepDegShIn, -sweepDegShIn);
                shadowPath.CloseFigure();
            }

            // Center hub shadow
            float shHub = deadzoneRadius + 5.0f;
            shadowPath.AddEllipse(cx - shHub, cy - shHub, shHub * 2.0f, shHub * 2.0f);

            Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(55, 0, 0, 0));
            g.FillPath(&shadowBrush, &shadowPath);
        }

        // 1. Draw sectors
        for (Int32 i = 0; i < count; ++i)
        {
            const PieSlot& slot = m_activeProfile->slots[i];
            if (!slot.enabled)
                continue;

            Bool isHovered = (m_hoveredSlot == i);

            float rIn = isHovered ? hoverInnerR : baseInnerR;
            float rOut = isHovered ? hoverOuterR : baseOuterR;

            float divDeg0 = 270.0f - sliceDeg * 0.5f + (float)i * sliceDeg;
            float startDegOut, sweepDegOut;
            float startDegIn, sweepDegIn;
            GetSectorAngles(rOut, divDeg0, startDegOut, sweepDegOut);
            GetSectorAngles(rIn,  divDeg0, startDegIn,  sweepDegIn);

            Gdiplus::GraphicsPath path;
            path.AddArc(cx - rOut, cy - rOut, rOut * 2.0f, rOut * 2.0f, startDegOut, sweepDegOut);
            path.AddArc(cx - rIn, cy - rIn, rIn * 2.0f, rIn * 2.0f, startDegIn + sweepDegIn, -sweepDegIn);
            path.CloseFigure();

            // Background color
            Gdiplus::Color bgCol;
            Bool hasAction = (slot.commandId > 0 || slot.IsSubMenu());
            if (isHovered)
            {
                BYTE r = (BYTE)LocalClampInt((int)(35 + slot.accentColor.x * 65.0), 0, 255);
                BYTE gr = (BYTE)LocalClampInt((int)(40 + slot.accentColor.y * 65.0), 0, 255);
                BYTE b = (BYTE)LocalClampInt((int)(50 + slot.accentColor.z * 65.0), 0, 255);
                bgCol = Gdiplus::Color(245, r, gr, b);
            }
            else if (hasAction)
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
            if (isHovered)
            {
                float stripW = 4.5f;
                float rStrip = rIn + stripW;
                float startDegStrip, sweepDegStrip;
                GetSectorAngles(rStrip, divDeg0, startDegStrip, sweepDegStrip);

                Gdiplus::GraphicsPath stripPath;
                stripPath.AddArc(cx - rStrip, cy - rStrip, rStrip * 2.0f, rStrip * 2.0f, startDegStrip, sweepDegStrip);
                stripPath.AddArc(cx - rIn, cy - rIn, rIn * 2.0f, rIn * 2.0f, startDegIn + sweepDegIn, -sweepDegIn);
                stripPath.CloseFigure();
                BYTE ar = (BYTE)(Clamp01(slot.accentColor.x) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot.accentColor.y) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot.accentColor.z) * 255.0);
                Gdiplus::SolidBrush stripBrush(Gdiplus::Color(255, ar, ag, ab));
                g.FillPath(&stripBrush, &stripPath);
            }
            else if (hasAction)
            {
                float stripW = 3.0f;
                float rStrip = rIn + stripW;
                float startDegStrip, sweepDegStrip;
                GetSectorAngles(rStrip, divDeg0, startDegStrip, sweepDegStrip);

                Gdiplus::GraphicsPath stripPath;
                stripPath.AddArc(cx - rStrip, cy - rStrip, rStrip * 2.0f, rStrip * 2.0f, startDegStrip, sweepDegStrip);
                stripPath.AddArc(cx - rIn, cy - rIn, rIn * 2.0f, rIn * 2.0f, startDegIn + sweepDegIn, -sweepDegIn);
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
            if (isHovered)
            {
                BYTE ar = (BYTE)(Clamp01(slot.accentColor.x) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot.accentColor.y) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot.accentColor.z) * 255.0);
                borderCol = Gdiplus::Color(255, ar, ag, ab);
                borderWidth = 2.0f;
            }
            else if (hasAction)
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

        // 2. Draw Icons and Labels inside segments
        Gdiplus::Font font(L"Segoe UI", 8.5f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBilinear);
        g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

        PieDisplayMode dispMode = m_activeProfile ? m_activeProfile->displayMode : mgr.m_displayMode;
        float baseIconSz = (float)(m_activeProfile ? ((m_activeProfile->iconSize >= 12.0) ? m_activeProfile->iconSize : 24.0) : ((mgr.m_iconSize >= 12.0) ? mgr.m_iconSize : 24.0));

        const PieHotkey* rootHk = (!m_profileStack.IsEmpty() && m_profileStack[0]) ? &m_profileStack[0]->hotkey : (m_activeProfile ? &m_activeProfile->hotkey : nullptr);
        bool actShift = m_shiftDown;
        bool actCtrl  = m_ctrlDown;
        bool actAlt   = m_altDown;
        if (rootHk && rootHk->enabled)
        {
            if (rootHk->shift) actShift = false;
            if (rootHk->ctrl)  actCtrl  = false;
            if (rootHk->alt)   actAlt   = false;
        }

        for (Int32 i = 0; i < count; ++i)
        {
            const PieSlot& slot = m_activeProfile->slots[i];
            if (!slot.enabled)
                continue;

            Bool isHovered = (m_hoveredSlot == i);

            float rIn = isHovered ? hoverInnerR : baseInnerR;
            float rOut = isHovered ? hoverOuterR : baseOuterR;
            float rMid = (rIn + rOut) * 0.5f;

            float midDeg = 270.0f + (float)i * sliceDeg;
            float midRad = midDeg * deg2rad;

            Int32 effectiveCmd = slot.GetCommandForModifiers(actShift, actCtrl, actAlt);
            Gdiplus::Bitmap* pIconBmp = nullptr;
            if (dispMode != PieDisplayMode::LabelOnly && effectiveCmd > 0)
            {
                pIconBmp = GetOrCreateGdiIcon(effectiveCmd);
            }

            String label = slot.GetEffectiveTitleForModifiers(actShift, actCtrl, actAlt);
            if (slot.IsSubMenu())
            {
                if (label.GetLength() > 9)
                {
                    label = label.SubStr(0, 8);
                    label += ".."_s;
                }
                label += " >"_s;
            }
            else if (pIconBmp != nullptr && dispMode == PieDisplayMode::IconAndLabel)
            {
                if (label.GetLength() > 11)
                {
                    label = label.SubStr(0, 10);
                    label += ".."_s;
                }
            }
            else
            {
                if (label.GetLength() > 12)
                {
                    label = label.SubStr(0, 11);
                    label += ".."_s;
                }
            }

            std::wstring wLabel = ToWString(label);

            Bool hasAction = (effectiveCmd > 0 || slot.IsSubMenu());
            Gdiplus::Color textCol;
            if (isHovered)
            {
                textCol = Gdiplus::Color(255, 255, 255, 255);
            }
            else if (hasAction)
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

            if (dispMode == PieDisplayMode::IconOnly && pIconBmp != nullptr)
            {
                // Only icon: centered at rMid, label is omitted
                float ix = cx + std::cos(midRad) * rMid;
                float iy = cy + std::sin(midRad) * rMid;

                float iconSz = isHovered ? (baseIconSz * 1.15f) : baseIconSz;
                if (iconSz > (rOut - rIn) * 0.85f)
                    iconSz = (rOut - rIn) * 0.85f;
                if (iconSz < 8.0f)
                    iconSz = 8.0f;

                if (isHovered)
                {
                    Gdiplus::SolidBrush iconGlow(Gdiplus::Color(55, (BYTE)(slot.accentColor.x * 255.0), (BYTE)(slot.accentColor.y * 255.0), (BYTE)(slot.accentColor.z * 255.0)));
                    g.FillEllipse(&iconGlow, ix - iconSz * 0.65f, iy - iconSz * 0.65f, iconSz * 1.3f, iconSz * 1.3f);
                }

                g.DrawImage(pIconBmp, ix - iconSz * 0.5f, iy - iconSz * 0.5f, iconSz, iconSz);
            }
            else if (dispMode == PieDisplayMode::IconAndLabel && pIconBmp != nullptr)
            {
                // Both icon and label: icon shifted inward, label shifted outward
                float offset = baseIconSz * 0.45f + 2.0f;
                float rIcon = rMid - offset;
                float rLabel = rMid + offset + 2.0f;

                float ix = cx + std::cos(midRad) * rIcon;
                float iy = cy + std::sin(midRad) * rIcon;

                float iconSz = isHovered ? (baseIconSz * 1.1f) : baseIconSz;
                if (iconSz > (rOut - rIn) * 0.85f)
                    iconSz = (rOut - rIn) * 0.85f;
                if (iconSz < 8.0f)
                    iconSz = 8.0f;

                if (isHovered)
                {
                    Gdiplus::SolidBrush iconGlow(Gdiplus::Color(45, (BYTE)(slot.accentColor.x * 255.0), (BYTE)(slot.accentColor.y * 255.0), (BYTE)(slot.accentColor.z * 255.0)));
                    g.FillEllipse(&iconGlow, ix - iconSz * 0.65f, iy - iconSz * 0.65f, iconSz * 1.3f, iconSz * 1.3f);
                }

                g.DrawImage(pIconBmp, ix - iconSz * 0.5f, iy - iconSz * 0.5f, iconSz, iconSz);

                float lx = cx + std::cos(midRad) * rLabel;
                float ly = cy + std::sin(midRad) * rLabel;

                // Drop shadow for text readability
                Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(180, 0, 0, 0));
                g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(lx + 1.0f, ly + 1.0f), &format, &shadowBrush);

                Gdiplus::SolidBrush textBrush(textCol);
                g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(lx, ly), &format, &textBrush);
            }
            else
            {
                // Label only (or slot without icon): centered at rMid
                float tx = cx + std::cos(midRad) * rMid;
                float ty = cy + std::sin(midRad) * rMid;

                // Drop shadow for text readability
                Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(180, 0, 0, 0));
                g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(tx + 1.0f, ty + 1.0f), &format, &shadowBrush);

                Gdiplus::SolidBrush textBrush(textCol);
                g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(tx, ty), &format, &textBrush);
            }
        }

        // 3. Center Hub (CENTER_SLOT_INDEX / deadzone)
        const PieSlot& slot8 = m_activeProfile->slots[CENTER_SLOT_INDEX];
        Int32 effectiveCenterCmd = slot8.GetCommandForModifiers(actShift, actCtrl, actAlt);
        Bool centerConfigured = (slot8.enabled && (effectiveCenterCmd > 0 || slot8.IsSubMenu()));
        Bool isSubMenuMode = !m_profileStack.IsEmpty();
        Bool centerHovered = (m_hoveredSlot == CENTER_SLOT_INDEX);

        Gdiplus::GraphicsPath hubPath;
        hubPath.AddEllipse(cx - deadzoneRadius, cy - deadzoneRadius, deadzoneRadius * 2.0f, deadzoneRadius * 2.0f);

        Gdiplus::Color hubBgCol;
        if (centerHovered)
        {
            if (centerConfigured)
            {
                BYTE r = (BYTE)LocalClampInt((int)(35 + slot8.accentColor.x * 65.0), 0, 255);
                BYTE gr = (BYTE)LocalClampInt((int)(40 + slot8.accentColor.y * 65.0), 0, 255);
                BYTE b = (BYTE)LocalClampInt((int)(50 + slot8.accentColor.z * 65.0), 0, 255);
                hubBgCol = Gdiplus::Color(245, r, gr, b);
            }
            else if (isSubMenuMode)
            {
                hubBgCol = Gdiplus::Color(245, 55, 65, 80);
            }
            else
            {
                hubBgCol = Gdiplus::Color(245, 45, 50, 60);
            }
        }
        else if (centerConfigured)
        {
            BYTE r = (BYTE)LocalClampInt((int)(26 + slot8.accentColor.x * 25.0), 0, 255);
            BYTE gr = (BYTE)LocalClampInt((int)(28 + slot8.accentColor.y * 25.0), 0, 255);
            BYTE b = (BYTE)LocalClampInt((int)(36 + slot8.accentColor.z * 25.0), 0, 255);
            hubBgCol = Gdiplus::Color(230, r, gr, b);
        }
        else if (isSubMenuMode)
        {
            hubBgCol = Gdiplus::Color(230, 36, 42, 52);
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
            if (centerConfigured)
            {
                BYTE ar = (BYTE)(Clamp01(slot8.accentColor.x) * 255.0);
                BYTE ag = (BYTE)(Clamp01(slot8.accentColor.y) * 255.0);
                BYTE ab = (BYTE)(Clamp01(slot8.accentColor.z) * 255.0);
                hubBorderCol = Gdiplus::Color(255, ar, ag, ab);
            }
            else if (isSubMenuMode)
            {
                hubBorderCol = Gdiplus::Color(255, 130, 180, 240);
            }
            else
            {
                hubBorderCol = Gdiplus::Color(220, 120, 130, 145);
            }
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
        else if (isSubMenuMode)
        {
            hubBorderCol = Gdiplus::Color(200, 90, 120, 160);
        }
        else
        {
            hubBorderCol = Gdiplus::Color(180, 80, 88, 100);
        }

        Gdiplus::Pen hubPen(hubBorderCol, centerHovered ? 2.0f : 1.2f);
        g.DrawPath(&hubPen, &hubPath);

        Gdiplus::Bitmap* pIconBmp8 = (dispMode != PieDisplayMode::LabelOnly && effectiveCenterCmd > 0) ? GetOrCreateGdiIcon(effectiveCenterCmd) : nullptr;

        if (centerConfigured && pIconBmp8 != nullptr)
        {
            float hubBaseSz = baseIconSz;
            float hubIconSz = (centerHovered ? (hubBaseSz * 1.1f) : hubBaseSz) * (deadzoneRadius / 26.0f);
            if (hubIconSz > deadzoneRadius * 1.6f)
                hubIconSz = deadzoneRadius * 1.6f;

            if (centerHovered)
            {
                Gdiplus::SolidBrush hubGlow(Gdiplus::Color(55, (BYTE)(slot8.accentColor.x * 255.0), (BYTE)(slot8.accentColor.y * 255.0), (BYTE)(slot8.accentColor.z * 255.0)));
                g.FillEllipse(&hubGlow, cx - hubIconSz * 0.65f, cy - hubIconSz * 0.65f, hubIconSz * 1.3f, hubIconSz * 1.3f);
            }

            g.DrawImage(pIconBmp8, cx - hubIconSz * 0.5f, cy - hubIconSz * 0.5f, hubIconSz, hubIconSz);
        }
        else if (centerConfigured)
        {
            String cText = slot8.GetEffectiveTitleForModifiers(m_shiftDown, m_ctrlDown, m_altDown);
            if (slot8.IsSubMenu())
                cText += " >"_s;
            std::wstring wCTitle = ToWString(cText);
            Gdiplus::Color cTextCol = centerHovered ? Gdiplus::Color(255, 255, 255, 255) :
                Gdiplus::Color(255, (BYTE)(slot8.accentColor.x * 255), (BYTE)(slot8.accentColor.y * 255), (BYTE)(slot8.accentColor.z * 255));
            Gdiplus::Font cFont(L"Segoe UI", 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush cTextBrush(cTextCol);
            g.DrawString(wCTitle.c_str(), -1, &cFont, Gdiplus::PointF(cx, cy), &format, &cTextBrush);
        }
        else if (isSubMenuMode)
        {
            std::wstring wCTitle = L"Back";
            Gdiplus::Color cTextCol = centerHovered ? Gdiplus::Color(255, 255, 255, 255) : Gdiplus::Color(240, 160, 180, 210);
            Gdiplus::Font cFont(L"Segoe UI", 7.5f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
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

        // 5. Context badge at top with breadcrumbs: e.g. "[Main > Submenu]"
        if (m_activeProfile->showTitle && m_activeProfile->name.IsPopulated())
        {
            std::wstring wBadge = L"[";
            for (const PieProfile* parentProf : m_profileStack)
            {
                if (parentProf)
                {
                    wBadge += ToWString(parentProf->name);
                    wBadge += L" > ";
                }
            }
            wBadge += ToWString(m_activeProfile->name);
            wBadge += L"]";

            Gdiplus::Font bFont(L"Segoe UI", 8.5f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush bShadow(Gdiplus::Color(180, 0, 0, 0));
            Gdiplus::SolidBrush bBrush(Gdiplus::Color(220, 130, 180, 230));

            float bY = cy - hoverOuterR - 22.0f;
            g.DrawString(wBadge.c_str(), -1, &bFont, Gdiplus::PointF(cx + 1.0f, bY + 1.0f), &format, &bShadow);
            g.DrawString(wBadge.c_str(), -1, &bFont, Gdiplus::PointF(cx, bY), &format, &bBrush);
        }

        // 6. Active modifier indicator badge
        std::wstring modTag;
        const PieSlot* hoveredSlotPtr = nullptr;
        if (m_hoveredSlot != NOTOK && m_hoveredSlot >= 0 && (m_hoveredSlot < count || m_hoveredSlot == CENTER_SLOT_INDEX))
        {
            hoveredSlotPtr = &m_activeProfile->slots[m_hoveredSlot];
        }

        if (actCtrl)
        {
            if (hoveredSlotPtr && hoveredSlotPtr->ctrlCommandId > 0)
                modTag += L"[CTRL] ";
            else
                modTag += L"[CTRL: Origin] ";
        }
        if (actAlt)
        {
            if (hoveredSlotPtr && hoveredSlotPtr->altCommandId > 0)
                modTag += L"[ALT] ";
            else
                modTag += L"[ALT: Parent] ";
        }
        if (actShift)
        {
            if (hoveredSlotPtr && hoveredSlotPtr->shiftCommandId > 0)
                modTag += L"[SHIFT] ";
            else
                modTag += L"[SHIFT: Child / Options] ";
        }
        if (!modTag.empty())
        {
            Gdiplus::Font mFont(L"Segoe UI", 8.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
            Gdiplus::SolidBrush mShadow(Gdiplus::Color(200, 0, 0, 0));
            Gdiplus::SolidBrush mBrush(Gdiplus::Color(255, 255, 210, 80)); // Warm gold badge
            float mY = cy + hoverOuterR + 18.0f;
            g.DrawString(modTag.c_str(), -1, &mFont, Gdiplus::PointF(cx + 1.0f, mY + 1.0f), &format, &mShadow);
            g.DrawString(modTag.c_str(), -1, &mFont, Gdiplus::PointF(cx, mY), &format, &mBrush);
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
