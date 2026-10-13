#include "piemenu_editor_area.h"
#include "piemenu_editor_dialog.h"
#include "piemenu_ids.h"
#include "windows_include.h"
#include <cmath>
#include <cstdint>
#include <utility>
#include "c4d_general.h"
#include "c4d_gui.h"

namespace cinema
{

PieMenuEditorArea::PieMenuEditorArea()
    : m_profileIndex(0), m_selectedSlot(0), m_dragHoverSlot(NOTOK), m_dragSourceSlot(NOTOK)
{
}

Float PieMenuEditorArea::GetDpiScale() const
{
    Float scale = 1.0;
#if defined(MAXON_TARGET_WINDOWS)
    UINT dpi = 0;
    HWND hwnd = nullptr;
    if (dlg)
    {
        hwnd = (HWND)dlg->GetWindowHandle();
    }
    if (hwnd)
    {
        typedef UINT (WINAPI *GetDpiForWindowFn)(HWND);
        static GetDpiForWindowFn pGetDpiForWindow = (GetDpiForWindowFn)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
        if (pGetDpiForWindow)
        {
            dpi = pGetDpiForWindow(hwnd);
        }
    }
    if (dpi == 0)
    {
        typedef UINT (WINAPI *GetDpiForSystemFn)();
        static GetDpiForSystemFn pGetDpiForSystem = (GetDpiForSystemFn)GetProcAddress(GetModuleHandleW(L"user32.dll"), "GetDpiForSystem");
        if (pGetDpiForSystem)
        {
            dpi = pGetDpiForSystem();
        }
    }
    if (dpi == 0)
    {
        HDC hdc = GetDC(NULL);
        if (hdc)
        {
            dpi = (UINT)GetDeviceCaps(hdc, LOGPIXELSX);
            ReleaseDC(NULL, hdc);
        }
    }
    if (dpi > 0)
    {
        scale = Float(dpi) / 96.0;
    }
#endif
    if (scale < 0.25)
        scale = 1.0;
    return scale;
}

Bool PieMenuEditorArea::GetMinSize(Int32& w, Int32& h)
{
    Float dpiScale = GetDpiScale();
    Float invDpi = (dpiScale > 0.05) ? (1.0 / dpiScale) : 1.0;
    w = (Int32)(360.0 * invDpi);
    h = (Int32)(320.0 * invDpi);
    if (w < 200) w = 200;
    if (h < 180) h = 180;
    return true;
}

void PieMenuEditorArea::SetActiveProfileIndex(Int32 index)
{
    m_profileIndex = index;
    m_selectedSlot = 0;
    m_dragHoverSlot = NOTOK;
    m_dragSourceSlot = NOTOK;
    Redraw();
}

void PieMenuEditorArea::SetSelectedSlot(Int32 slot)
{
    m_selectedSlot = slot;
    Redraw();
}

Int32 PieMenuEditorArea::GetSlotUnderCoord(Int32 x, Int32 y) const
{
    Int32 w = GetWidth();
    Int32 h = GetHeight();
    Float cx = Float(w) * 0.5;
    Float cy = Float(h) * 0.5;
    Float dx = Float(x) - cx;
    Float dy = Float(y) - cy;
    Float dist = std::sqrt(dx * dx + dy * dy);

    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
    if (!prof)
        return NOTOK;

    Float dpiScale = GetDpiScale();
    Float invDpi = (dpiScale > 0.05) ? (1.0 / dpiScale) : 1.0;

    Float outRVal = prof->outerRadius;
    Float dzVal = prof->deadzone;
    Float deadzoneRadius = ((dzVal > 0.0) ? dzVal : 26.0) * invDpi;
    Float maxOuterR = (outRVal + 15.0) * invDpi;

    if (dist < deadzoneRadius)
        return CENTER_SLOT_INDEX; // Center Slot

    if (dist > maxOuterR)
        return NOTOK;

    Float angleRad = std::atan2(dy, dx);
    Float deg = angleRad * (180.0 / PI);
    Int32 slot = prof->AngleToSlot(deg, dist, deadzoneRadius);
    if (slot >= 0 && slot < prof->slotCount)
        return slot;

    return NOTOK;
}

Int32 PieMenuEditorArea::ExtractCommandId(Int32 type, void* object, const BaseContainer& msg)
{
    if (type == DRAGTYPE_COMMAND)
    {
        uintptr_t intVal = reinterpret_cast<uintptr_t>(object);
        if (intVal > 0 && intVal <= 0x7FFFFFFF)
        {
            String name = GetCommandName((Int32)intVal);
            if (name.IsPopulated())
                return (Int32)intVal;
        }

        Int32 msgId = msg.GetInt32(BFM_DRAG_DATA_NEW, NOTOK);
        if (msgId > 0)
        {
            String name = GetCommandName(msgId);
            if (name.IsPopulated())
                return msgId;
        }

        msgId = msg.GetInt32(BFM_DRAG_DATA_, NOTOK);
        if (msgId > 0)
        {
            String name = GetCommandName(msgId);
            if (name.IsPopulated())
                return msgId;
        }

        if (object != nullptr)
        {
            __try
            {
                Int32 deref = *reinterpret_cast<const Int32*>(object);
                if (deref > 0)
                    return deref;
            }
            __except (1) // EXCEPTION_EXECUTE_HANDLER
            {
            }
        }

        if (intVal > 0 && intVal <= 0x7FFFFFFF)
            return (Int32)intVal;
    }
    return NOTOK;
}

maxon::String PieMenuEditorArea::ExtractSubmenuName(Int32 type, void* object, const BaseContainer& msg)
{
    if (type != DRAGTYPE_PIEMENU_SUBMENU)
        return ""_s;

    PieProfileManager& mgr = PieProfileManager::Get();

    // 1. Try object as SubmenuItem*
    if (object != nullptr)
    {
        __try
        {
            const SubmenuItem* item = reinterpret_cast<const SubmenuItem*>(object);
            if (item && item->name.IsPopulated())
            {
                PieProfile* p = mgr.FindProfileByName(item->name);
                if (p)
                    return p->name;
            }
        }
        __except (1)
        {
        }
    }

    // 2. Try object as profile index (e.g. uintptr_t)
    uintptr_t intVal = reinterpret_cast<uintptr_t>(object);
    if (intVal < (uintptr_t)mgr.GetProfileCount())
    {
        PieProfile* p = mgr.GetProfile((Int32)intVal);
        if (p)
            return p->name;
    }

    // 3. Try container messages
    Int32 msgIdx = msg.GetInt32(BFM_DRAG_DATA_NEW, NOTOK);
    if (msgIdx >= 0 && msgIdx < mgr.GetProfileCount())
    {
        PieProfile* p = mgr.GetProfile(msgIdx);
        if (p)
            return p->name;
    }

    msgIdx = msg.GetInt32(BFM_DRAG_DATA_, NOTOK);
    if (msgIdx >= 0 && msgIdx < mgr.GetProfileCount())
    {
        PieProfile* p = mgr.GetProfile(msgIdx);
        if (p)
            return p->name;
    }

    return ""_s;
}

void PieMenuEditorArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
    OffScreenOn();
    SetClippingRegion(x1, y1, x2, y2);

    Int32 w = GetWidth();
    Int32 h = GetHeight();

    // Background (standard window color)
    DrawSetPen(COLOR_BG);
    DrawRectangle(0, 0, w, h);

    const Vector defaultBg = DrawGetPenColor(COLOR_BG);

    // Frame
    DrawSetPen(COLOR_EDGEDK);
    DrawFrame(0, 0, w - 1, h - 1);

    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
    if (!prof)
    {
        DrawSetPen(Vector(0.45, 0.48, 0.52));
        DrawSetTextCol(Vector(0.55, 0.58, 0.62), COLOR_TRANS);
        String emptyMsg = "No Pie Menu (Click '+ New' to create one)"_s;
        Int32 tw = DrawGetTextWidth(emptyMsg);
        Int32 th = DrawGetFontHeight();
        DrawText(emptyMsg, (w - tw) / 2, (h - th) / 2);
        return;
    }

    Float cx = Float(w) * 0.5;
    Float cy = Float(h) * 0.5;

    // Lambda: draw filled annular ring sector (donut slice) using polygon fill
    auto drawAnnularSectorPoly = [&](Float cenX, Float cenY, Float rIn, Float rOut, Float aStartIn, Float aEndIn, Float aStartOut, Float aEndOut, const Vector& fillColor, Int32 steps = 14)
    {
        Vector2d pts[64];
        Int32 idx = 0;
        for (Int32 s = 0; s <= steps && idx < 32; ++s)
        {
            Float a = aStartOut + (Float(s) / Float(steps)) * (aEndOut - aStartOut);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rOut, cenY + std::sin(a) * rOut);
        }
        for (Int32 s = steps; s >= 0 && idx < 64; --s)
        {
            Float a = aStartIn + (Float(s) / Float(steps)) * (aEndIn - aStartIn);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rIn, cenY + std::sin(a) * rIn);
        }
        DrawSetPen(fillColor);
        DrawPolyFill(maxon::Block<const Vector2d>(pts, idx), true);
    };

    // Lambda: draw contour lines of an annular sector
    auto drawAnnularSectorBorder = [&](Float cenX, Float cenY, Float rIn, Float rOut, Float aStartIn, Float aEndIn, Float aStartOut, Float aEndOut, const Vector& borderColor, Float lineWidth, Int32 steps = 14)
    {
        Vector2d pts[64];
        Int32 idx = 0;
        for (Int32 s = 0; s <= steps && idx < 32; ++s)
        {
            Float a = aStartOut + (Float(s) / Float(steps)) * (aEndOut - aStartOut);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rOut, cenY + std::sin(a) * rOut);
        }
        for (Int32 s = steps; s >= 0 && idx < 64; --s)
        {
            Float a = aStartIn + (Float(s) / Float(steps)) * (aEndIn - aStartIn);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rIn, cenY + std::sin(a) * rIn);
        }
        DrawSetPen(borderColor);
        DrawPolyLine(maxon::Block<const Vector2d>(pts, idx), true, lineWidth);
    };

    PieProfileManager& mgr = PieProfileManager::Get();

    Float dpiScale = GetDpiScale();
    Float invDpi = (dpiScale > 0.05) ? (1.0 / dpiScale) : 1.0;

    // Circular layout dimensions matching real pie menu dimensions (compensated for GUI DPI scaling)
    Float outRVal = prof ? prof->outerRadius : mgr.m_outerRadius;
    Float inRVal = prof ? prof->innerRadius : mgr.m_innerRadius;
    Float dzVal = prof ? prof->deadzone : mgr.m_deadzone;
    const Float deadzoneRadius = ((dzVal > 0.0) ? dzVal : 26.0) * invDpi;
    const Float baseInnerR = inRVal * invDpi;
    const Float baseOuterR = outRVal * invDpi;
    const Float hoverInnerR = ((inRVal > 3.0) ? (inRVal - 3.0) : inRVal) * invDpi;
    const Float hoverOuterR = (outRVal + 13.0) * invDpi; // Pops out by 13px on selection / drag-hover
    Float opRaw = prof ? prof->opacity : mgr.m_opacity;
    const Float opVal = (opRaw < 0.1) ? 0.1 : ((opRaw > 1.0) ? 1.0 : opRaw);

    Int32 count = prof->slotCount;
    if (count < MIN_SLOT_COUNT) count = MIN_SLOT_COUNT;
    if (count > MAX_SLOT_COUNT) count = MAX_SLOT_COUNT;

    const Float deg2rad = PI / 180.0;
    const Float sliceDeg = 360.0 / Float(count);
    const Float sliceRad = sliceDeg * deg2rad;
    const Float gapPx = ((count <= 4) ? 3.0 : ((count <= 8) ? 2.5 : ((count <= 14) ? 2.0 : 1.6))) * invDpi;
    const Float halfGapDist = gapPx * 0.5;
    const Int32 steps = (count <= 4) ? 18 : ((count <= 8) ? 14 : ((count <= 14) ? 10 : 7));

    auto GetSectorAnglesRad = [sliceRad, halfGapDist](Float r, Float div0, Float& outStart, Float& outEnd)
    {
        Float dAng = 0.0;
        if (r > halfGapDist && r > 0.1)
        {
            Float ratio = halfGapDist / r;
            if (ratio > 0.999) ratio = 0.999;
            dAng = std::asin(ratio);
            if (dAng > sliceRad * 0.48)
                dAng = sliceRad * 0.48;
        }
        outStart = div0 + dAng;
        outEnd = div0 + sliceRad - dAng;
    };

    // 1. Draw circular segments (Fills and active indicators)
    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = prof->slots[i];
        Bool isSelected = (m_selectedSlot == i);
        Bool isDragHover = (m_dragHoverSlot == i);
        Bool isDragSource = (m_dragSourceSlot == i);
        Bool isSelectedOrHovered = (isSelected || isDragHover || isDragSource);

        Float rIn = isSelectedOrHovered ? hoverInnerR : baseInnerR;
        Float rOut = isSelectedOrHovered ? hoverOuterR : baseOuterR;

        Float divRad0 = (270.0 - sliceDeg * 0.5 + Float(i) * sliceDeg) * deg2rad;
        Float aStartIn, aEndIn, aStartOut, aEndOut;
        GetSectorAnglesRad(rIn,  divRad0, aStartIn,  aEndIn);
        GetSectorAnglesRad(rOut, divRad0, aStartOut, aEndOut);

        Vector bgCol;
        if (isDragHover)
        {
            bgCol = Vector(0.22, 0.24, 0.28) + slot.accentColor * 0.20;
        }
        else if (isDragSource)
        {
            bgCol = Vector(0.18, 0.22, 0.30) + slot.accentColor * 0.20;
        }
        else if (isSelected)
        {
            bgCol = Vector(0.11, 0.13, 0.17) + slot.accentColor * 0.25;
        }
        else if (!slot.enabled)
        {
            bgCol = defaultBg; // matches area background (hole)
        }
        else if (slot.commandId > 0 || slot.IsSubMenu())
        {
            bgCol = Vector(0.11, 0.13, 0.17) + slot.accentColor * 0.08;
        }
        else
        {
            bgCol = Vector(0.09, 0.10, 0.13);
        }

        if (slot.enabled && opVal < 0.999)
        {
            bgCol = bgCol * opVal + defaultBg * (1.0 - opVal);
        }

        drawAnnularSectorPoly(cx, cy, rIn, rOut, aStartIn, aEndIn, aStartOut, aEndOut, bgCol, steps);

        // Active / Selected illuminated inner arc strip
        Float stripW = (isSelectedOrHovered ? 4.5 : 3.0) * invDpi;
        if (stripW < 1.5) stripW = 1.5;
        Float rStrip = rIn + stripW;
        Float aStartStrip, aEndStrip;
        GetSectorAnglesRad(rStrip, divRad0, aStartStrip, aEndStrip);

        if (isDragHover)
        {
            drawAnnularSectorPoly(cx, cy, rIn, rStrip, aStartIn, aEndIn, aStartStrip, aEndStrip, Vector(1.0, 1.0, 1.0), steps);
        }
        else if (isDragSource)
        {
            drawAnnularSectorPoly(cx, cy, rIn, rStrip, aStartIn, aEndIn, aStartStrip, aEndStrip, Vector(0.25, 0.70, 1.0), steps);
        }
        else if (isSelected)
        {
            drawAnnularSectorPoly(cx, cy, rIn, rStrip, aStartIn, aEndIn, aStartStrip, aEndStrip, slot.accentColor, steps);
        }
        else if (slot.enabled && (slot.commandId > 0 || slot.IsSubMenu()))
        {
            drawAnnularSectorPoly(cx, cy, rIn, rStrip, aStartIn, aEndIn, aStartStrip, aEndStrip, slot.accentColor * 0.65, steps);
        }
    }

    // 2. Draw Borders around segments
    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = prof->slots[i];
        Bool isSelected = (m_selectedSlot == i);
        Bool isDragHover = (m_dragHoverSlot == i);
        Bool isDragSource = (m_dragSourceSlot == i);
        Bool isSelectedOrHovered = (isSelected || isDragHover || isDragSource);

        Float rIn = isSelectedOrHovered ? hoverInnerR : baseInnerR;
        Float rOut = isSelectedOrHovered ? hoverOuterR : baseOuterR;

        Float divRad0 = (270.0 - sliceDeg * 0.5 + Float(i) * sliceDeg) * deg2rad;
        Float aStartIn, aEndIn, aStartOut, aEndOut;
        GetSectorAnglesRad(rIn,  divRad0, aStartIn,  aEndIn);
        GetSectorAnglesRad(rOut, divRad0, aStartOut, aEndOut);

        Vector borderCol;
        Float borderWidth = 1.0;
        if (isDragHover)
        {
            borderCol = Vector(1.0, 1.0, 1.0); // Bright white drag highlight
            borderWidth = 2.5;
        }
        else if (isDragSource)
        {
            borderCol = Vector(0.25, 0.70, 1.0); // Electric cyan source highlight
            borderWidth = 2.2;
        }
        else if (isSelected)
        {
            borderCol = slot.accentColor; // Selection highlight in slot's own accent color
            borderWidth = 2.2;
        }
        else if (!slot.enabled)
        {
            borderCol = Vector(0.17, 0.17, 0.19); // Subtle ghost outline for hole
            borderWidth = 1.0;
        }
        else if (slot.commandId > 0 || slot.IsSubMenu())
        {
            borderCol = slot.accentColor * 0.65;
            borderWidth = 1.2;
        }
        else
        {
            borderCol = Vector(0.24, 0.27, 0.31);
            borderWidth = 1.0;
        }

        if (slot.enabled && opVal < 0.999 && !isSelected && !isDragHover && !isDragSource)
        {
            borderCol = borderCol * opVal + defaultBg * (1.0 - opVal);
        }

        drawAnnularSectorBorder(cx, cy, rIn, rOut, aStartIn, aEndIn, aStartOut, aEndOut, borderCol, borderWidth, steps);
    }

    // 3. Draw Labels and Icons inside each segment
    PieDisplayMode dispMode = prof ? prof->displayMode : mgr.m_displayMode;
    Float baseIconSz = prof ? prof->iconSize : mgr.m_iconSize;
    if (baseIconSz < 12.0)
        baseIconSz = 24.0;
    if (baseIconSz > 64.0)
        baseIconSz = 64.0;

    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = prof->slots[i];
        Bool isSelected = (m_selectedSlot == i);
        Bool isDragHover = (m_dragHoverSlot == i);
        Bool isDragSource = (m_dragSourceSlot == i);
        Bool isSelectedOrHovered = (isSelected || isDragHover || isDragSource);

        Float rIn = isSelectedOrHovered ? hoverInnerR : baseInnerR;
        Float rOut = isSelectedOrHovered ? hoverOuterR : baseOuterR;
        Float rMid = (rIn + rOut) * 0.5;

        Float midDeg = 270.0 + Float(i) * sliceDeg;
        Float midRad = midDeg * deg2rad;

        IconData icon;
        Bool hasIcon = (dispMode != PieDisplayMode::LabelOnly && slot.commandId > 0 && slot.GetIconBitmap(icon) && icon.bmp != nullptr);

        String label = slot.GetEffectiveTitle();
        if (slot.IsSubMenu())
        {
            if (label.GetLength() > 8)
            {
                label = label.SubStr(0, 7);
                label += ".."_s;
            }
            label += " >"_s;
        }
        else if (label.GetLength() > 10)
        {
            label = label.SubStr(0, 9);
            label += ".."_s;
        }

        Vector textCol;
        if (isDragHover)
            textCol = Vector(1.0, 1.0, 1.0);
        else if (isDragSource || isSelected)
            textCol = Vector(1.0, 1.0, 1.0);
        else if (!slot.enabled)
            textCol = Vector(0.30, 0.32, 0.35); // Dim ghost text
        else if (slot.commandId > 0 || slot.IsSubMenu())
            textCol = slot.accentColor;
        else
            textCol = Vector(0.65, 0.68, 0.72);

        Bool isVisible = (slot.enabled || isSelected || isDragHover || isDragSource || slot.commandId > 0 || slot.IsSubMenu());

        Float hoverScale = (dispMode == PieDisplayMode::IconAndLabel) ? 1.1 : 1.15;
        Float curIconSz = (isSelectedOrHovered ? (baseIconSz * hoverScale) : baseIconSz) * invDpi;
        if (curIconSz > (rOut - rIn) * 0.85)
            curIconSz = (rOut - rIn) * 0.85;
        if (curIconSz < 8.0)
            curIconSz = 8.0;
        Int32 sz = (Int32)curIconSz;

        Vector bgCol;
        if (isDragHover)
            bgCol = Vector(0.22, 0.24, 0.28) + slot.accentColor * 0.20;
        else if (isDragSource)
            bgCol = Vector(0.18, 0.22, 0.30) + slot.accentColor * 0.20;
        else if (isSelected)
            bgCol = Vector(0.11, 0.13, 0.17) + slot.accentColor * 0.25;
        else if (!slot.enabled)
            bgCol = defaultBg;
        else if (slot.commandId > 0 || slot.IsSubMenu())
            bgCol = Vector(0.11, 0.13, 0.17) + slot.accentColor * 0.08;
        else
            bgCol = Vector(0.09, 0.10, 0.13);

        Int32 drawflags = BMP_ALLOWALPHA | BMP_TRANSPARENTALPHA | BMP_NORMALSCALED;
        if (icon.flags & ICONDATAFLAGS::APPLYCOLORPROFILE)
            drawflags |= BMP_APPLY_COLORPROFILE;

        if (dispMode == PieDisplayMode::IconOnly && hasIcon && (slot.enabled || isSelected || isDragHover || isDragSource))
        {
            // Only icon: centered at rMid
            Float ix = cx + std::cos(midRad) * rMid;
            Float iy = cy + std::sin(midRad) * rMid;
            DrawSetPen(bgCol);
            DrawBitmap(icon.bmp, (Int32)(ix - sz * 0.5), (Int32)(iy - sz * 0.5), sz, sz, icon.x, icon.y, icon.w, icon.h, drawflags);
        }
        else if (dispMode == PieDisplayMode::IconAndLabel && hasIcon && (slot.enabled || isSelected || isDragHover || isDragSource))
        {
            // Position icon slightly closer to inner ring, text slightly closer to outer ring
            Float offset = (baseIconSz * 0.45 + 2.0) * invDpi;
            Float rIcon = rMid - offset;
            Float rLabel = rMid + offset + 2.0 * invDpi;

            Float ix = cx + std::cos(midRad) * rIcon;
            Float iy = cy + std::sin(midRad) * rIcon;
            DrawSetPen(bgCol);
            DrawBitmap(icon.bmp, (Int32)(ix - sz * 0.5), (Int32)(iy - sz * 0.5), sz, sz, icon.x, icon.y, icon.w, icon.h, drawflags);

            Float lx = cx + std::cos(midRad) * rLabel;
            Float ly = cy + std::sin(midRad) * rLabel;
            DrawSetTextCol(textCol, COLOR_TRANS);
            DrawText(label, (Int32)lx, (Int32)(ly - 5), DRAWTEXT_HALIGN_CENTER);
        }
        else if (isVisible)
        {
            // Center label inside sector (LabelOnly, or slot without icon, or submenu)
            Float lx = cx + std::cos(midRad) * rMid;
            Float ly = cy + std::sin(midRad) * rMid;
            DrawSetTextCol(textCol, COLOR_TRANS);
            DrawText(label, (Int32)lx, (Int32)(ly - 5), DRAWTEXT_HALIGN_CENTER);
        }
    }

    // 4. Direction line pointing to selected slot
    if (m_selectedSlot >= 0 && m_selectedSlot < count)
    {
        Float selMidDeg = 270.0 + Float(m_selectedSlot) * sliceDeg;
        Float selMidRad = selMidDeg * deg2rad;
        Vector selCol = (m_dragHoverSlot == m_selectedSlot) ? Vector(1.0, 1.0, 1.0) : prof->slots[m_selectedSlot].accentColor;
        DrawSetPen(selCol);
        Float sx = cx + std::cos(selMidRad) * (deadzoneRadius + 2.0 * invDpi);
        Float sy = cy + std::sin(selMidRad) * (deadzoneRadius + 2.0 * invDpi);
        Float ex = cx + std::cos(selMidRad) * (hoverInnerR - 2.0 * invDpi);
        Float ey = cy + std::sin(selMidRad) * (hoverInnerR - 2.0 * invDpi);
        DrawLine((Int32)sx, (Int32)sy, (Int32)ex, (Int32)ey, 1.5);
    }

    // 5. Center Hub (CENTER_SLOT_INDEX)
    const PieSlot& slot8 = prof->slots[CENTER_SLOT_INDEX];
    Bool isSelected8 = (m_selectedSlot == CENTER_SLOT_INDEX);
    Bool isDragHover8 = (m_dragHoverSlot == CENTER_SLOT_INDEX);
    Bool isDragSource8 = (m_dragSourceSlot == CENTER_SLOT_INDEX);

    Vector hubBgCol;
    Vector hubBorderCol;
    Float hubBorderWidth = 1.0;

    if (isDragHover8)
    {
        hubBgCol = Vector(0.22, 0.24, 0.28) + slot8.accentColor * 0.20;
        hubBorderCol = Vector(1.0, 1.0, 1.0);
        hubBorderWidth = 2.5;
    }
    else if (isDragSource8)
    {
        hubBgCol = Vector(0.18, 0.22, 0.30) + slot8.accentColor * 0.20;
        hubBorderCol = Vector(0.25, 0.70, 1.0);
        hubBorderWidth = 2.2;
    }
    else if (isSelected8)
    {
        hubBgCol = Vector(0.11, 0.13, 0.17) + slot8.accentColor * 0.25;
        hubBorderCol = slot8.accentColor;
        hubBorderWidth = 2.2;
    }
    else if (!slot8.enabled)
    {
        hubBgCol = defaultBg; // matches area background
        hubBorderCol = Vector(0.18, 0.18, 0.20);
        hubBorderWidth = 1.0;
    }
    else if (slot8.commandId > 0 || slot8.IsSubMenu())
    {
        hubBgCol = Vector(0.11, 0.13, 0.17) + slot8.accentColor * 0.08;
        hubBorderCol = slot8.accentColor * 0.65;
        hubBorderWidth = 1.2;
    }
    else
    {
        hubBgCol = Vector(0.10, 0.11, 0.14);
        hubBorderCol = Vector(0.32, 0.35, 0.40);
        hubBorderWidth = 1.0;
    }

    if (slot8.enabled && opVal < 0.999)
    {
        hubBgCol = hubBgCol * opVal + defaultBg * (1.0 - opVal);
    }
    if (slot8.enabled && opVal < 0.999 && !isSelected8 && !isDragHover8 && !isDragSource8)
    {
        hubBorderCol = hubBorderCol * opVal + defaultBg * (1.0 - opVal);
    }

    DrawSetPen(hubBgCol);
    DrawEllipseFill(Vector2d(cx, cy), Vector2d(deadzoneRadius, deadzoneRadius));

    DrawSetPen(hubBorderCol);
    DrawEllipseLine(Vector2d(cx, cy), Vector2d(deadzoneRadius, deadzoneRadius), hubBorderWidth);

    if ((slot8.commandId > 0 || slot8.IsSubMenu()) && (slot8.enabled || isSelected8 || isDragHover8 || isDragSource8))
    {
        IconData icon;
        Bool hasIcon8 = (dispMode != PieDisplayMode::LabelOnly && slot8.commandId > 0 && slot8.GetIconBitmap(icon) && icon.bmp != nullptr);

        if (hasIcon8)
        {
            Float hoverScale = (dispMode == PieDisplayMode::IconAndLabel) ? 1.1 : 1.15;
            Float hubIconSz = (isSelected8 || isDragHover8 || isDragSource8 ? (baseIconSz * hoverScale) : baseIconSz) * invDpi;
            if (hubIconSz < 8.0)
                hubIconSz = 8.0;
            Int32 sz = (Int32)hubIconSz;

            Int32 drawflags = BMP_ALLOWALPHA | BMP_TRANSPARENTALPHA | BMP_NORMALSCALED;
            if (icon.flags & ICONDATAFLAGS::APPLYCOLORPROFILE)
                drawflags |= BMP_APPLY_COLORPROFILE;
            DrawSetPen(hubBgCol);
            DrawBitmap(icon.bmp, (Int32)(cx - sz * 0.5), (Int32)(cy - sz * 0.5), sz, sz, icon.x, icon.y, icon.w, icon.h, drawflags);
        }
        else
        {
            DrawSetTextCol((isSelected8 || isDragHover8 || isDragSource8) ? Vector(1.0) : slot8.accentColor, COLOR_TRANS);
            String hubLabel = slot8.GetEffectiveTitle();
            if (slot8.IsSubMenu())
                hubLabel += " >"_s;
            if (hubLabel.GetLength() > 8)
            {
                hubLabel = hubLabel.SubStr(0, 7);
                hubLabel += ".."_s;
            }
            DrawText(hubLabel, (Int32)cx, (Int32)(cy - 5), DRAWTEXT_HALIGN_CENTER);
        }
    }
    else
    {
        // Crosshair at center
        Float chSize = (4.0 * invDpi < deadzoneRadius * 0.35) ? (4.0 * invDpi) : (deadzoneRadius * 0.35);
        if (chSize < 2.0) chSize = 2.0;
        DrawSetPen(isSelected8 ? slot8.accentColor : Vector(0.50, 0.54, 0.60));
        DrawLine((Int32)(cx - chSize), (Int32)cy, (Int32)(cx + chSize), (Int32)cy, 1.0);
        DrawLine((Int32)cx, (Int32)(cy - chSize), (Int32)cx, (Int32)(cy + chSize), 1.0);

        DrawSetTextCol(isSelected8 ? slot8.accentColor : Vector(0.50, 0.54, 0.60), COLOR_TRANS);
        Float textOff = (7.0 * invDpi < deadzoneRadius * 0.3) ? (7.0 * invDpi) : (deadzoneRadius * 0.3);
        DrawText((!slot8.enabled && !isSelected8) ? "Off"_s : "Hub"_s, (Int32)cx, (Int32)(cy + textOff), DRAWTEXT_HALIGN_CENTER);
    }

    // 6. Dynamic visual swap/move connection link between source and target slots
    auto getSlotCenter = [&](Int32 s) -> Vector2d
    {
        if (s == CENTER_SLOT_INDEX)
            return Vector2d(cx, cy);
        if (s >= 0 && s < count)
        {
            Float midDeg = 270.0 + Float(s) * sliceDeg;
            Float midRad = midDeg * deg2rad;
            Float rMid = (baseInnerR + baseOuterR) * 0.5;
            return Vector2d(cx + std::cos(midRad) * rMid, cy + std::sin(midRad) * rMid);
        }
        return Vector2d(cx, cy);
    };

    if (m_dragSourceSlot != NOTOK && m_dragHoverSlot != NOTOK && m_dragHoverSlot != m_dragSourceSlot)
    {
        Vector2d ptSrc = getSlotCenter(m_dragSourceSlot);
        Vector2d ptDst = getSlotCenter(m_dragHoverSlot);

#if defined(MAXON_TARGET_WINDOWS)
        bool ctrlHeld = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                        ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                        ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
#else
        bool ctrlHeld = false;
#endif

        Vector lineCol = ctrlHeld ? Vector(0.3, 0.95, 0.5) : Vector(1.0, 1.0, 1.0);
        DrawSetPen(lineCol);
        DrawLine((Int32)ptSrc.x, (Int32)ptSrc.y, (Int32)ptDst.x, (Int32)ptDst.y, 2.0);

        // Center badge on the connection line
        Float midX = (ptSrc.x + ptDst.x) * 0.5;
        Float midY = (ptSrc.y + ptDst.y) * 0.5;
        DrawSetPen(Vector(0.10, 0.11, 0.14));
        DrawEllipseFill(Vector2d(midX, midY), Vector2d(10.0, 10.0));
        DrawSetPen(lineCol);
        DrawEllipseLine(Vector2d(midX, midY), Vector2d(10.0, 10.0), 1.5);
        DrawSetTextCol(lineCol, COLOR_TRANS);
        DrawText(ctrlHeld ? "+"_s : "<>"_s, (Int32)midX, (Int32)(midY - 5.0), DRAWTEXT_HALIGN_CENTER);
    }

    // 7. Context badge at top (if enabled for this menu)
    if (prof->showTitle && prof->name.IsPopulated())
    {
        String badge = "["_s + prof->name + "]"_s;
        DrawSetTextCol(Vector(0.50, 0.70, 0.90), COLOR_TRANS);
        DrawText(badge, (Int32)cx, (Int32)(cy - hoverOuterR - 16.0 * invDpi), DRAWTEXT_HALIGN_CENTER);
    }

    // 8. Bottom hint
    Int32 hintY = h - 22;
    if (m_dragSourceSlot != NOTOK && m_dragHoverSlot != NOTOK && m_dragHoverSlot != m_dragSourceSlot)
    {
#if defined(MAXON_TARGET_WINDOWS)
        bool ctrlHeld = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                        ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                        ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
#else
        bool ctrlHeld = false;
#endif
        if (ctrlHeld)
        {
            DrawSetTextCol(Vector(0.3, 0.95, 0.5), COLOR_TRANS);
            DrawText("Release to duplicate slot to target"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
        }
        else
        {
            DrawSetTextCol(Vector(1.0, 1.0, 1.0), COLOR_TRANS);
            DrawText("Release to swap slots | Hold Ctrl to duplicate"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
        }
    }
    else if (m_dragSourceSlot != NOTOK)
    {
        DrawSetTextCol(Vector(0.5, 0.8, 1.0), COLOR_TRANS);
        DrawText("Drag to another slot to move or swap"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
    }
    else if (m_dragHoverSlot != NOTOK)
    {
#if defined(MAXON_TARGET_WINDOWS)
        bool shiftHeld = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
        bool ctrlHeld = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
        bool altHeld = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);
#else
        bool shiftHeld = false, ctrlHeld = false, altHeld = false;
#endif
        if (shiftHeld)
        {
            DrawSetTextCol(Vector(0.3, 0.95, 0.5), COLOR_TRANS);
            DrawText("Release to assign as [SHIFT] action"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
        }
        else if (ctrlHeld)
        {
            DrawSetTextCol(Vector(0.3, 0.95, 0.5), COLOR_TRANS);
            DrawText("Release to assign as [CTRL] action"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
        }
        else if (altHeld)
        {
            DrawSetTextCol(Vector(0.3, 0.95, 0.5), COLOR_TRANS);
            DrawText("Release to assign as [ALT] action"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
        }
        else
        {
            DrawSetTextCol(Vector(1.0, 1.0, 1.0), COLOR_TRANS);
            DrawText("Release to assign | Hold Shift/Ctrl/Alt for modifier action"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
        }
    }
    else
    {
        DrawSetTextCol(Vector(0.52, 0.55, 0.60), COLOR_TRANS);
        DrawText("Click slot to inspect | Drag slots to rearrange | Drag command or submenu to assign"_s, (Int32)cx, hintY, DRAWTEXT_HALIGN_CENTER);
    }
}

Bool PieMenuEditorArea::InputEvent(const BaseContainer& msg)
{
    Int32 dev = msg.GetInt32(BFM_INPUT_DEVICE);
    Int32 chn = msg.GetInt32(BFM_INPUT_CHANNEL);

    if (dev == BFM_INPUT_MOUSE && chn == BFM_INPUT_MOUSELEFT)
    {
        Int32 mx = msg.GetInt32(BFM_INPUT_X);
        Int32 my = msg.GetInt32(BFM_INPUT_Y);
        Global2Local(&mx, &my);

        Int32 slot = GetSlotUnderCoord(mx, my);
        if (slot != NOTOK)
        {
            m_selectedSlot = slot;
            Redraw();

            BaseContainer action(BFM_ACTION);
            action.SetInt32(BFM_ACTION_ID, GetId());
            SendParentMessage(action);

            PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
            if (prof)
            {
                m_dragSourceSlot = slot;
                m_dragHoverSlot = NOTOK;
                HandleMouseDrag(msg, DRAGTYPE_PIEMENU_SLOT, reinterpret_cast<void*>(static_cast<uintptr_t>(slot)), 0);
                m_dragSourceSlot = NOTOK;
                m_dragHoverSlot = NOTOK;
                Redraw();
            }
            return true;
        }
    }

    return GeUserArea::InputEvent(msg);
}

Int32 PieMenuEditorArea::Message(const BaseContainer& msg, BaseContainer& result)
{
    switch (msg.GetId())
    {
        case BFM_DRAGRECEIVE:
        {
            Int32 mx = msg.GetInt32(BFM_DRAG_SCREENX);
            Int32 my = msg.GetInt32(BFM_DRAG_SCREENY);
            Screen2Local(&mx, &my);

            Int32 hoverSlot = GetSlotUnderCoord(mx, my);

            if (msg.GetInt32(BFM_DRAG_LOST) || msg.GetInt32(BFM_DRAG_ESC))
            {
                m_dragHoverSlot = NOTOK;
                m_dragSourceSlot = NOTOK;
                Redraw();
                return GeUserArea::Message(msg, result);
            }

            Int32 type = 0;
            void* object = nullptr;
            GetDragObject(msg, &type, &object);

            if (type == DRAGTYPE_PIEMENU_SLOT || (type == DRAGTYPE_COMMAND && m_dragSourceSlot != NOTOK))
            {
                if (hoverSlot != m_dragHoverSlot)
                {
                    m_dragHoverSlot = hoverSlot;
                    Redraw();
                }

                if (msg.GetInt32(BFM_DRAG_FINISHED))
                {
                    Int32 srcSlot = m_dragSourceSlot;
                    if (srcSlot == NOTOK)
                    {
                        uintptr_t val = reinterpret_cast<uintptr_t>(object);
                        if (val < TOTAL_SLOT_COUNT)
                            srcSlot = (Int32)val;
                    }

                    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
                    if (prof && srcSlot != NOTOK && hoverSlot != NOTOK && srcSlot != hoverSlot)
                    {
                        Bool validSrc = (srcSlot >= 0 && (srcSlot < prof->slotCount || srcSlot == CENTER_SLOT_INDEX));
                        Bool validDst = (hoverSlot >= 0 && (hoverSlot < prof->slotCount || hoverSlot == CENTER_SLOT_INDEX));

                        if (validSrc && validDst)
                        {
#if defined(MAXON_TARGET_WINDOWS)
                            bool ctrlHeld = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) ||
                                            ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) ||
                                            ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
#else
                            bool ctrlHeld = false;
#endif
                            if (ctrlHeld)
                            {
                                prof->slots[hoverSlot] = prof->slots[srcSlot];
                            }
                            else
                            {
                                std::swap(prof->slots[srcSlot], prof->slots[hoverSlot]);
                            }

                            m_selectedSlot = hoverSlot;

                            BaseContainer action(BFM_ACTION);
                            action.SetInt32(BFM_ACTION_ID, GetId());
                            SendParentMessage(action);
                        }
                    }

                    m_dragHoverSlot = NOTOK;
                    m_dragSourceSlot = NOTOK;
                    Redraw();
                }

                return SetDragDestination(MOUSE_POINT_HAND);
            }
            else if (type == DRAGTYPE_COMMAND)
            {
                if (hoverSlot != m_dragHoverSlot)
                {
                    m_dragHoverSlot = hoverSlot;
                    Redraw();
                }

                if (msg.GetInt32(BFM_DRAG_FINISHED))
                {
                    Int32 cmdId = ExtractCommandId(type, object, msg);
                    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
                    if (prof && cmdId > 0 && m_dragHoverSlot != NOTOK && m_dragHoverSlot >= 0 && (m_dragHoverSlot < prof->slotCount || m_dragHoverSlot == CENTER_SLOT_INDEX))
                    {
#if defined(MAXON_TARGET_WINDOWS)
                        bool shiftHeld = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RSHIFT) & 0x8000) != 0);
                        bool ctrlHeld = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LCONTROL) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RCONTROL) & 0x8000) != 0);
                        bool altHeld = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_LMENU) & 0x8000) != 0) || ((GetAsyncKeyState(VK_RMENU) & 0x8000) != 0);
#else
                        bool shiftHeld = false, ctrlHeld = false, altHeld = false;
#endif
                        if (shiftHeld)
                        {
                            prof->slots[m_dragHoverSlot].shiftCommandId = cmdId;
                        }
                        else if (ctrlHeld)
                        {
                            prof->slots[m_dragHoverSlot].ctrlCommandId = cmdId;
                        }
                        else if (altHeld)
                        {
                            prof->slots[m_dragHoverSlot].altCommandId = cmdId;
                        }
                        else
                        {
                            prof->slots[m_dragHoverSlot].commandId = cmdId;
                            prof->slots[m_dragHoverSlot].subMenuName = ""_s;
                            prof->slots[m_dragHoverSlot].customLabel = GetCommandName(cmdId);
                        }

                        prof->slots[m_dragHoverSlot].enabled = true;
                        m_selectedSlot = m_dragHoverSlot;

                        BaseContainer action(BFM_ACTION);
                        action.SetInt32(BFM_ACTION_ID, GetId());
                        SendParentMessage(action);
                    }
                    m_dragHoverSlot = NOTOK;
                    Redraw();
                }

                return SetDragDestination(MOUSE_POINT_HAND);
            }
            else if (type == DRAGTYPE_PIEMENU_SUBMENU)
            {
                if (hoverSlot != m_dragHoverSlot)
                {
                    m_dragHoverSlot = hoverSlot;
                    Redraw();
                }

                if (msg.GetInt32(BFM_DRAG_FINISHED))
                {
                    maxon::String subName = ExtractSubmenuName(type, object, msg);
                    PieProfileManager& mgr = PieProfileManager::Get();
                    PieProfile* subProf = mgr.FindProfileByName(subName);
                    PieProfile* prof = mgr.GetProfile(m_profileIndex);
                    if (subProf && prof && m_dragHoverSlot != NOTOK && m_dragHoverSlot >= 0 && (m_dragHoverSlot < prof->slotCount || m_dragHoverSlot == CENTER_SLOT_INDEX))
                    {
                        prof->slots[m_dragHoverSlot].subMenuName = subProf->name;
                        prof->slots[m_dragHoverSlot].commandId = NOTOK;
                        prof->slots[m_dragHoverSlot].customLabel = subProf->name;
                        prof->slots[m_dragHoverSlot].enabled = true;
                        m_selectedSlot = m_dragHoverSlot;

                        BaseContainer action(BFM_ACTION);
                        action.SetInt32(BFM_ACTION_ID, GetId());
                        SendParentMessage(action);
                    }
                    m_dragHoverSlot = NOTOK;
                    Redraw();
                }

                return SetDragDestination(MOUSE_POINT_HAND);
            }
            break;
        }
    }

    return GeUserArea::Message(msg, result);
}

} // namespace cinema
