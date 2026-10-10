#include "piemenu_editor_area.h"
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

Bool PieMenuEditorArea::GetMinSize(Int32& w, Int32& h)
{
    w = 360;
    h = 360;
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

    if (dist < 32.0)
        return 8; // Center Slot

    if (dist > 165.0)
        return NOTOK;

    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
    if (!prof)
        return NOTOK;

    Float angleRad = std::atan2(dy, dx);
    Float deg = angleRad * (180.0 / PI);
    Int32 slot = prof->AngleToSlot(deg, dist, 32.0);
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

void PieMenuEditorArea::DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg)
{
    OffScreenOn();
    SetClippingRegion(x1, y1, x2, y2);

    Int32 w = GetWidth();
    Int32 h = GetHeight();

    // Background
    DrawSetPen(Vector(0.12, 0.13, 0.16));
    DrawRectangle(0, 0, w, h);

    // Frame
    DrawSetPen(Vector(0.22, 0.24, 0.28));
    DrawFrame(0, 0, w - 1, h - 1);

    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
    if (!prof)
        return;

    Float cx = Float(w) * 0.5;
    Float cy = Float(h) * 0.5;

    // Lambda: draw filled annular ring sector (donut slice) using polygon fill
    auto drawAnnularSectorPoly = [&](Float cenX, Float cenY, Float rIn, Float rOut, Float aStart, Float aEnd, const Vector& fillColor, Int32 steps = 14)
    {
        Vector2d pts[64];
        Int32 idx = 0;
        for (Int32 s = 0; s <= steps && idx < 32; ++s)
        {
            Float a = aStart + (Float(s) / Float(steps)) * (aEnd - aStart);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rOut, cenY + std::sin(a) * rOut);
        }
        for (Int32 s = steps; s >= 0 && idx < 64; --s)
        {
            Float a = aStart + (Float(s) / Float(steps)) * (aEnd - aStart);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rIn, cenY + std::sin(a) * rIn);
        }
        DrawSetPen(fillColor);
        DrawPolyFill(maxon::Block<const Vector2d>(pts, idx), true);
    };

    // Lambda: draw contour lines of an annular sector
    auto drawAnnularSectorBorder = [&](Float cenX, Float cenY, Float rIn, Float rOut, Float aStart, Float aEnd, const Vector& borderColor, Float lineWidth, Int32 steps = 14)
    {
        Vector2d pts[64];
        Int32 idx = 0;
        for (Int32 s = 0; s <= steps && idx < 32; ++s)
        {
            Float a = aStart + (Float(s) / Float(steps)) * (aEnd - aStart);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rOut, cenY + std::sin(a) * rOut);
        }
        for (Int32 s = steps; s >= 0 && idx < 64; --s)
        {
            Float a = aStart + (Float(s) / Float(steps)) * (aEnd - aStart);
            pts[idx++] = Vector2d(cenX + std::cos(a) * rIn, cenY + std::sin(a) * rIn);
        }
        DrawSetPen(borderColor);
        DrawPolyLine(maxon::Block<const Vector2d>(pts, idx), true, lineWidth);
    };

    // Circular layout dimensions matching QuadDraw style
    const Float deadzoneRadius = 30.0;
    const Float baseInnerR = 40.0;
    const Float baseOuterR = 142.0;
    const Float hoverInnerR = 36.0;
    const Float hoverOuterR = 152.0; // Pops out by 10px on selection / drag-hover

    Int32 count = prof->slotCount;
    if (count != 4 && count != 8)
        count = 8;

    const Float deg2rad = PI / 180.0;
    const Float sliceDeg = 360.0 / Float(count);
    const Float gapDeg = (count == 4) ? 2.5 : 2.0;
    const Float gapRad = (gapDeg * deg2rad) * 0.5;
    const Int32 steps = (count == 4) ? 18 : 14;

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

        Float startDeg = 270.0 - sliceDeg * 0.5 + Float(i) * sliceDeg;
        Float aStart = startDeg * deg2rad + gapRad;
        Float aEnd = (startDeg + sliceDeg) * deg2rad - gapRad;

        Vector bgCol;
        if (isDragHover)
        {
            bgCol = Vector(0.24, 0.22, 0.12) + slot.accentColor * 0.25;
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
            bgCol = Vector(0.12, 0.12, 0.13); // matches area background (hole)
        }
        else if (slot.commandId > 0)
        {
            bgCol = Vector(0.11, 0.13, 0.17) + slot.accentColor * 0.08;
        }
        else
        {
            bgCol = Vector(0.09, 0.10, 0.13);
        }

        drawAnnularSectorPoly(cx, cy, rIn, rOut, aStart, aEnd, bgCol, steps);

        // Active / Selected illuminated inner arc strip
        if (isDragHover)
        {
            drawAnnularSectorPoly(cx, cy, rIn, rIn + 5.0, aStart, aEnd, Vector(1.0, 0.9, 0.15), steps);
        }
        else if (isDragSource)
        {
            drawAnnularSectorPoly(cx, cy, rIn, rIn + 5.0, aStart, aEnd, Vector(0.25, 0.70, 1.0), steps);
        }
        else if (isSelected)
        {
            drawAnnularSectorPoly(cx, cy, rIn, rIn + 5.0, aStart, aEnd, slot.accentColor, steps);
        }
        else if (slot.enabled && slot.commandId > 0)
        {
            drawAnnularSectorPoly(cx, cy, rIn, rIn + 3.5, aStart, aEnd, slot.accentColor * 0.65, steps);
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

        Float startDeg = 270.0 - sliceDeg * 0.5 + Float(i) * sliceDeg;
        Float aStart = startDeg * deg2rad + gapRad;
        Float aEnd = (startDeg + sliceDeg) * deg2rad - gapRad;

        Vector borderCol;
        Float borderWidth = 1.0;
        if (isDragHover)
        {
            borderCol = Vector(1.0, 0.9, 0.15); // Bright gold drag highlight
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
        else if (slot.commandId > 0)
        {
            borderCol = slot.accentColor * 0.65;
            borderWidth = 1.2;
        }
        else
        {
            borderCol = Vector(0.24, 0.27, 0.31);
            borderWidth = 1.0;
        }

        drawAnnularSectorBorder(cx, cy, rIn, rOut, aStart, aEnd, borderCol, borderWidth, steps);
    }

    // 3. Draw Labels and Icons inside each segment
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
        Bool hasIcon = (slot.commandId > 0 && slot.GetIconBitmap(icon) && icon.bmp != nullptr);

        String label = slot.GetEffectiveTitle();
        if (label.GetLength() > 10)
        {
            label = label.SubStr(0, 9);
            label += ".."_s;
        }

        Vector textCol;
        if (isDragHover)
            textCol = Vector(1.0, 0.95, 0.3);
        else if (isDragSource || isSelected)
            textCol = Vector(1.0, 1.0, 1.0);
        else if (!slot.enabled)
            textCol = Vector(0.30, 0.32, 0.35); // Dim ghost text
        else if (slot.commandId > 0)
            textCol = slot.accentColor;
        else
            textCol = Vector(0.65, 0.68, 0.72);

        if (hasIcon && (slot.enabled || isSelected || isDragHover || isDragSource))
        {
            // Position icon slightly closer to inner ring, text slightly closer to outer ring
            Float rIcon = rMid - 13.0;
            Float rLabel = rMid + 15.0;

            Float ix = cx + std::cos(midRad) * rIcon;
            Float iy = cy + std::sin(midRad) * rIcon;
            DrawBitmap(icon.bmp, (Int32)(ix - 10), (Int32)(iy - 10), 20, 20, icon.x, icon.y, icon.w, icon.h, BMP_ALLOWALPHA);

            Float lx = cx + std::cos(midRad) * rLabel;
            Float ly = cy + std::sin(midRad) * rLabel;
            DrawSetTextCol(textCol, COLOR_TRANS);
            DrawText(label, (Int32)lx, (Int32)(ly - 5), DRAWTEXT_HALIGN_CENTER);
        }
        else
        {
            // Center label inside sector (or omit empty label if disabled and unselected)
            if (slot.enabled || isSelected || isDragHover || isDragSource || slot.commandId > 0)
            {
                Float lx = cx + std::cos(midRad) * rMid;
                Float ly = cy + std::sin(midRad) * rMid;
                DrawSetTextCol(textCol, COLOR_TRANS);
                DrawText(label, (Int32)lx, (Int32)(ly - 5), DRAWTEXT_HALIGN_CENTER);
            }
        }
    }

    // 4. Direction line pointing to selected slot
    if (m_selectedSlot >= 0 && m_selectedSlot < count)
    {
        Float selMidDeg = 270.0 + Float(m_selectedSlot) * sliceDeg;
        Float selMidRad = selMidDeg * deg2rad;
        Vector selCol = (m_dragHoverSlot == m_selectedSlot) ? Vector(1.0, 0.9, 0.15) : prof->slots[m_selectedSlot].accentColor;
        DrawSetPen(selCol);
        Float sx = cx + std::cos(selMidRad) * (deadzoneRadius + 2.0);
        Float sy = cy + std::sin(selMidRad) * (deadzoneRadius + 2.0);
        Float ex = cx + std::cos(selMidRad) * (hoverInnerR - 2.0);
        Float ey = cy + std::sin(selMidRad) * (hoverInnerR - 2.0);
        DrawLine((Int32)sx, (Int32)sy, (Int32)ex, (Int32)ey, 1.5);
    }

    // 5. Center Hub (Slot 8)
    const PieSlot& slot8 = prof->slots[8];
    Bool isSelected8 = (m_selectedSlot == 8);
    Bool isDragHover8 = (m_dragHoverSlot == 8);
    Bool isDragSource8 = (m_dragSourceSlot == 8);

    Vector hubBgCol;
    Vector hubBorderCol;
    Float hubBorderWidth = 1.0;

    if (isDragHover8)
    {
        hubBgCol = Vector(0.24, 0.22, 0.12) + slot8.accentColor * 0.25;
        hubBorderCol = Vector(1.0, 0.9, 0.15);
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
        hubBgCol = Vector(0.12, 0.12, 0.13); // matches area background
        hubBorderCol = Vector(0.18, 0.18, 0.20);
        hubBorderWidth = 1.0;
    }
    else if (slot8.commandId > 0)
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

    DrawSetPen(hubBgCol);
    DrawEllipseFill(Vector2d(cx, cy), Vector2d(deadzoneRadius, deadzoneRadius));

    DrawSetPen(hubBorderCol);
    DrawEllipseLine(Vector2d(cx, cy), Vector2d(deadzoneRadius, deadzoneRadius), hubBorderWidth);

    if (slot8.commandId > 0 && (slot8.enabled || isSelected8 || isDragHover8 || isDragSource8))
    {
        IconData icon;
        if (slot8.GetIconBitmap(icon) && icon.bmp)
        {
            DrawBitmap(icon.bmp, (Int32)(cx - 10), (Int32)(cy - 10), 20, 20, icon.x, icon.y, icon.w, icon.h, BMP_ALLOWALPHA);
        }
        else
        {
            DrawSetTextCol((isSelected8 || isDragHover8 || isDragSource8) ? Vector(1.0) : slot8.accentColor, COLOR_TRANS);
            DrawText("Center"_s, (Int32)cx, (Int32)(cy - 5), DRAWTEXT_HALIGN_CENTER);
        }
    }
    else
    {
        // Crosshair at center
        DrawSetPen(isSelected8 ? slot8.accentColor : Vector(0.50, 0.54, 0.60));
        DrawLine((Int32)(cx - 3.5), (Int32)cy, (Int32)(cx + 3.5), (Int32)cy, 1.0);
        DrawLine((Int32)cx, (Int32)(cy - 3.5), (Int32)cx, (Int32)(cy + 3.5), 1.0);

        DrawSetTextCol(isSelected8 ? slot8.accentColor : Vector(0.50, 0.54, 0.60), COLOR_TRANS);
        DrawText((!slot8.enabled && !isSelected8) ? "Off"_s : "Hub"_s, (Int32)cx, (Int32)(cy + 7), DRAWTEXT_HALIGN_CENTER);
    }

    // 6. Dynamic visual swap/move connection link between source and target slots
    auto getSlotCenter = [&](Int32 s) -> Vector2d
    {
        if (s == 8)
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

        Vector lineCol = ctrlHeld ? Vector(0.3, 0.95, 0.5) : Vector(1.0, 0.85, 0.2);
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
        DrawText(badge, (Int32)cx, (Int32)(cy - hoverOuterR - 16.0), DRAWTEXT_HALIGN_CENTER);
    }

    // 8. Bottom hint
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
            DrawText("Release to duplicate slot to target"_s, (Int32)cx, h - 14, DRAWTEXT_HALIGN_CENTER);
        }
        else
        {
            DrawSetTextCol(Vector(1.0, 0.88, 0.25), COLOR_TRANS);
            DrawText("Release to swap slots | Hold Ctrl to duplicate"_s, (Int32)cx, h - 14, DRAWTEXT_HALIGN_CENTER);
        }
    }
    else if (m_dragSourceSlot != NOTOK)
    {
        DrawSetTextCol(Vector(0.5, 0.8, 1.0), COLOR_TRANS);
        DrawText("Drag to another slot to move or swap"_s, (Int32)cx, h - 14, DRAWTEXT_HALIGN_CENTER);
    }
    else if (m_dragHoverSlot != NOTOK)
    {
        DrawSetTextCol(Vector(1.0, 0.88, 0.25), COLOR_TRANS);
        DrawText("Release to assign command to slot"_s, (Int32)cx, h - 14, DRAWTEXT_HALIGN_CENTER);
    }
    else
    {
        DrawSetTextCol(Vector(0.52, 0.55, 0.60), COLOR_TRANS);
        DrawText("Click slot to inspect | Drag slots to rearrange | Drag command to assign"_s, (Int32)cx, h - 14, DRAWTEXT_HALIGN_CENTER);
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
            if (prof && (prof->slots[slot].commandId > 0 || prof->slots[slot].customLabel.IsPopulated()))
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
                        if (val <= 8)
                            srcSlot = (Int32)val;
                    }

                    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
                    if (prof && srcSlot != NOTOK && hoverSlot != NOTOK && srcSlot != hoverSlot)
                    {
                        Bool validSrc = (srcSlot >= 0 && (srcSlot < prof->slotCount || srcSlot == 8));
                        Bool validDst = (hoverSlot >= 0 && (hoverSlot < prof->slotCount || hoverSlot == 8));

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
                    if (cmdId > 0 && m_dragHoverSlot != NOTOK && m_dragHoverSlot >= 0 && m_dragHoverSlot < 9)
                    {
                        PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
                        if (prof)
                        {
                            prof->slots[m_dragHoverSlot].commandId = cmdId;
                            prof->slots[m_dragHoverSlot].customLabel = GetCommandName(cmdId);
                            prof->slots[m_dragHoverSlot].enabled = true;
                            m_selectedSlot = m_dragHoverSlot;

                            BaseContainer action(BFM_ACTION);
                            action.SetInt32(BFM_ACTION_ID, GetId());
                            SendParentMessage(action);
                        }
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
