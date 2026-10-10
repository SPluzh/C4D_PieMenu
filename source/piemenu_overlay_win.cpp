#include "piemenu_overlay_win.h"
#include "piemenu_editor_dialog.h"
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

void PieMenuOverlay::CommitAndClose()
{
#if defined(MAXON_TARGET_WINDOWS)
    if (!m_active)
        return;

    if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < 9)
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (slot.enabled && slot.IsSubMenu())
        {
            PieProfile* sub = PieProfileManager::Get().FindProfileByName(slot.subMenuName);
            if (sub && CanNavigateTo(sub))
            {
                NavigateToSubmenu(sub);
                m_pinnedOpen = true;
                return;
            }
        }
        else if (m_hoveredSlot == 8 && !m_profileStack.IsEmpty() && (!slot.enabled || (slot.commandId <= 0 && !slot.IsSubMenu())))
        {
            NavigateBack();
            return;
        }
    }
    else if (!m_profileStack.IsEmpty())
    {
        // Check if inside center deadzone (Back button)
        Float dx = Float(m_currentScreenX - m_originScreenX);
        Float dy = Float(m_currentScreenY - m_originScreenY);
        Float dist = std::sqrt(dx * dx + dy * dy);
        PieProfileManager& mgr = PieProfileManager::Get();
        const Float deadzone = (mgr.m_deadzone > 0.0) ? mgr.m_deadzone : 26.0;
        if (dist < deadzone)
        {
            NavigateBack();
            return;
        }

        // Inside a submenu, if user released trigger or clicked empty space,
        // pin the submenu open so the user can see choices and select them
        if (!m_pinnedOpen)
        {
            m_pinnedOpen = true;
            return;
        }
    }

    Int32 commandToCall = NOTOK;
    String slotTitle = "None"_s;

    if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < 9)
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (slot.enabled && slot.commandId > 0 && !slot.IsSubMenu())
        {
            commandToCall = slot.commandId;
            slotTitle = slot.GetEffectiveTitle();
        }
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
    // When navigated into a submenu hierarchy, the trigger hotkey being held
    // is ALWAYS the hotkey of the root menu profile that opened the pie menu session!
    if (!m_profileStack.IsEmpty() && m_profileStack[0])
    {
        return IsHotkeyTriggered(m_profileStack[0]->hotkey);
    }

    if (m_activeProfile)
    {
        return IsHotkeyTriggered(m_activeProfile->hotkey);
    }

    return false;
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
            CommitAndClose();
        }
    }
    else
    {
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

            // 2. Global context fallback
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
            // 3. Unique hotkey match: if only one profile matched the pressed shortcut, trigger it regardless of mode
            if (!triggered && matched.GetCount() == 1)
            {
                triggered = matched[0];
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
        if (m_activeProfile->slots[8].enabled && (m_activeProfile->slots[8].commandId > 0 || m_activeProfile->slots[8].IsSubMenu()))
        {
            m_hoveredSlot = 8;
        }
        else if (!m_profileStack.IsEmpty())
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
        Int32 s = m_activeProfile->AngleToSlot(deg, dist, deadzone);
        Int32 count = m_activeProfile->slotCount;
        if (count != 4 && count != 8)
            count = 8;
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
    }

    if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < 9)
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (m_hoveredSlot == 8 && !m_profileStack.IsEmpty() && (!slot.enabled || (slot.commandId <= 0 && !slot.IsSubMenu())))
        {
            StatusSetText("Pie Menu: [Back to parent menu] (Click or release to go back)"_s);
        }
        else if (slot.IsSubMenu())
        {
            StatusSetText(FormatString("Pie Menu [@]: Submenu > @ | Hover or click to open"_s, m_activeProfile->name, slot.GetEffectiveTitle()));
        }
        else
        {
            StatusSetText(FormatString("Pie Menu [@]: @ | Release trigger to execute"_s, m_activeProfile->name, slot.GetEffectiveTitle()));
        }
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

        case WM_LBUTTONDOWN:
        {
            // Do not commit on mouse down; wait for WM_LBUTTONUP so click executes once
            return 0;
        }

        case WM_LBUTTONUP:
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
                    CommitAndClose();
                    return 0;
                }

                // Check auto-expand submenu on hover
                if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < 9)
                {
                    const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
                    if (slot.enabled && slot.IsSubMenu() && m_hoverStartTick > 0)
                    {
                        UInt32 elapsed = (UInt32)GetTickCount() - m_hoverStartTick;
                        if (elapsed >= 240)
                        {
                            PieProfile* sub = PieProfileManager::Get().FindProfileByName(slot.subMenuName);
                            if (sub && CanNavigateTo(sub))
                            {
                                NavigateToSubmenu(sub);
                                return 0;
                            }
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

        // 0. Soft radial ambient backdrop shadow behind visible slots and center hub
        {
            Gdiplus::GraphicsPath shadowPath;
            for (Int32 i = 0; i < count; ++i)
            {
                if (!m_activeProfile->slots[i].enabled)
                    continue;

                float rIn = (m_hoveredSlot == i) ? hoverInnerR : baseInnerR;
                float rOut = (m_hoveredSlot == i) ? hoverOuterR : baseOuterR;
                float shIn = rIn - 4.0f * radiusScale;
                if (shIn < 0.0f)
                    shIn = 0.0f;
                float shOut = rOut + 8.0f * radiusScale;

                float startDeg = 270.0f - sliceDeg * 0.5f + (float)i * sliceDeg + gapDeg * 0.5f;
                float sweepDeg = sliceDeg - gapDeg;

                shadowPath.AddArc(cx - shOut, cy - shOut, shOut * 2.0f, shOut * 2.0f, startDeg, sweepDeg);
                shadowPath.AddArc(cx - shIn, cy - shIn, shIn * 2.0f, shIn * 2.0f, startDeg + sweepDeg, -sweepDeg);
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

            float startDeg = 270.0f - sliceDeg * 0.5f + (float)i * sliceDeg + gapDeg * 0.5f;
            float sweepDeg = sliceDeg - gapDeg;

            Gdiplus::GraphicsPath path;
            path.AddArc(cx - rOut, cy - rOut, rOut * 2.0f, rOut * 2.0f, startDeg, sweepDeg);
            path.AddArc(cx - rIn, cy - rIn, rIn * 2.0f, rIn * 2.0f, startDeg + sweepDeg, -sweepDeg);
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
            else if (hasAction)
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

        // 2. Draw Labels inside segments
        Gdiplus::Font font(L"Segoe UI", 9.0f, Gdiplus::FontStyleBold, Gdiplus::UnitPoint);
        Gdiplus::StringFormat format;
        format.SetAlignment(Gdiplus::StringAlignmentCenter);
        format.SetLineAlignment(Gdiplus::StringAlignmentCenter);

        for (Int32 i = 0; i < count; ++i)
        {
            const PieSlot& slot = m_activeProfile->slots[i];
            if (!slot.enabled)
                continue;

            Bool isHovered = (m_hoveredSlot == i);

            float rIn = isHovered ? hoverInnerR : baseInnerR;
            float rOut = isHovered ? hoverOuterR : baseOuterR;
            float rText = (rIn + rOut) * 0.5f;

            float midDeg = 270.0f + (float)i * sliceDeg;
            float midRad = midDeg * deg2rad;

            float tx = cx + std::cos(midRad) * rText;
            float ty = cy + std::sin(midRad) * rText;

            String label = slot.GetEffectiveTitle();
            if (slot.IsSubMenu())
                label += " >"_s;
            std::wstring wLabel = ToWString(label);

            Bool hasAction = (slot.commandId > 0 || slot.IsSubMenu());
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

            // Drop shadow for text readability
            Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(180, 0, 0, 0));
            g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(tx + 1.0f, ty + 1.0f), &format, &shadowBrush);

            Gdiplus::SolidBrush textBrush(textCol);
            g.DrawString(wLabel.c_str(), -1, &font, Gdiplus::PointF(tx, ty), &format, &textBrush);
        }

        // 3. Center Hub (Slot 8 / deadzone)
        const PieSlot& slot8 = m_activeProfile->slots[8];
        Bool centerConfigured = (slot8.enabled && (slot8.commandId > 0 || slot8.IsSubMenu()));
        Bool isSubMenuMode = !m_profileStack.IsEmpty();
        Bool centerHovered = (m_hoveredSlot == 8);

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

        if (centerConfigured)
        {
            String cText = slot8.GetEffectiveTitle();
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
            std::wstring wCTitle = L"◀ Back";
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
