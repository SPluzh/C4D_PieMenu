#include "piemenu_editor_area.h"
#include <cmath>
#include <cstdint>
#include "c4d_general.h"
#include "c4d_gui.h"

namespace cinema
{

PieMenuEditorArea::PieMenuEditorArea()
    : m_profileIndex(0), m_selectedSlot(0), m_dragHoverSlot(NOTOK)
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

    if (dist > 185.0)
        return NOTOK;

    PieProfile* prof = PieProfileManager::Get().GetProfile(m_profileIndex);
    if (!prof)
        return NOTOK;

    Float angleRad = std::atan2(dy, dx);
    Float deg = angleRad * (180.0 / PI);
    return prof->AngleToSlot(deg, dist, 32.0);
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
    Float radius = 118.0;
    Float deadzone = 32.0;

    Int32 count = prof->slotCount;
    if (count != 4 && count != 8)
        count = 8;

    // Draw spokes and radial slots
    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = prof->slots[i];
        Float angle = prof->GetSlotAngle(i);
        Float sx = cx + std::cos(angle) * radius;
        Float sy = cy + std::sin(angle) * radius;

        Bool isSelected = (m_selectedSlot == i);
        Bool isDragHover = (m_dragHoverSlot == i);

        // Spoke line
        Vector spokeCol = (isSelected || isDragHover) ? slot.accentColor : Vector(0.24, 0.26, 0.30);
        DrawSetPen(spokeCol);
        Float spkStartX = cx + std::cos(angle) * (deadzone + 2.0);
        Float spkStartY = cy + std::sin(angle) * (deadzone + 2.0);
        DrawLine((Int32)spkStartX, (Int32)spkStartY, (Int32)sx, (Int32)sy, 1.0);

        // Card box (88 x 32 px)
        Int32 cw = 88;
        Int32 ch = 32;
        Int32 bx1 = (Int32)(sx - cw * 0.5);
        Int32 by1 = (Int32)(sy - ch * 0.5);
        Int32 bx2 = bx1 + cw;
        Int32 by2 = by1 + ch;

        Vector bgCol;
        if (!slot.enabled)
            bgCol = Vector(0.08, 0.08, 0.09);
        else if (isDragHover)
            bgCol = Vector(0.32, 0.28, 0.14);
        else if (isSelected)
            bgCol = Vector(0.18, 0.26, 0.38);
        else
            bgCol = Vector(0.16, 0.17, 0.20);

        DrawSetPen(bgCol);
        DrawRectangle(bx1, by1, bx2, by2);

        Vector frameCol;
        if (isDragHover)
            frameCol = Vector(1.0, 0.9, 0.15);
        else if (isSelected)
            frameCol = Vector(0.3, 0.85, 1.0);
        else if (slot.enabled)
            frameCol = slot.accentColor * 0.75;
        else
            frameCol = Vector(0.24, 0.24, 0.26);

        DrawSetPen(frameCol);
        DrawFrame(bx1, by1, bx2, by2, (isSelected || isDragHover) ? 2.0 : 1.0);

        // Draw Icon if available
        Int32 textOffsetX = 8;
        if (slot.commandId > 0)
        {
            IconData icon;
            if (slot.GetIconBitmap(icon) && icon.bmp)
            {
                DrawBitmap(icon.bmp, bx1 + 4, by1 + 4, 24, 24, icon.x, icon.y, icon.w, icon.h, BMP_ALLOWALPHA);
                textOffsetX = 32;
            }
        }

        // Draw Title text
        String label = slot.GetEffectiveTitle();
        if (label.GetLength() > 9)
        {
            // Truncate long titles to fit cleanly in editor card
            label = label.SubStr(0, 8);
            label += ".."_s;
        }

        Vector textCol = (isSelected || isDragHover) ? Vector(1.0) : (slot.enabled ? Vector(0.85, 0.88, 0.92) : Vector(0.45));
        DrawSetTextCol(textCol, COLOR_TRANS);
        DrawText(label, bx1 + textOffsetX, by1 + 10, DRAWTEXT_HALIGN_LEFT);
    }

    // Draw Center Slot (Slot 8)
    {
        const PieSlot& slot8 = prof->slots[8];
        Bool isSelected8 = (m_selectedSlot == 8);
        Bool isDragHover8 = (m_dragHoverSlot == 8);

        Int32 r8 = 30;
        Int32 bx1 = (Int32)(cx - r8);
        Int32 by1 = (Int32)(cy - r8);
        Int32 bx2 = bx1 + r8 * 2;
        Int32 by2 = by1 + r8 * 2;

        Vector bgCol8;
        if (isDragHover8)
            bgCol8 = Vector(0.32, 0.28, 0.14);
        else if (isSelected8)
            bgCol8 = Vector(0.18, 0.26, 0.38);
        else if (!slot8.enabled)
            bgCol8 = Vector(0.08, 0.08, 0.09);
        else
            bgCol8 = Vector(0.15, 0.16, 0.19);

        DrawSetPen(bgCol8);
        DrawRectangle(bx1, by1, bx2, by2);

        Vector frameCol8;
        if (isDragHover8)
            frameCol8 = Vector(1.0, 0.9, 0.15);
        else if (isSelected8)
            frameCol8 = Vector(0.3, 0.85, 1.0);
        else if (slot8.enabled && slot8.commandId > 0)
            frameCol8 = slot8.accentColor * 0.75;
        else
            frameCol8 = Vector(0.3, 0.32, 0.36);

        DrawSetPen(frameCol8);
        DrawFrame(bx1, by1, bx2, by2, (isSelected8 || isDragHover8) ? 2.0 : 1.0);

        if (slot8.commandId > 0)
        {
            IconData icon;
            if (slot8.GetIconBitmap(icon) && icon.bmp)
            {
                DrawBitmap(icon.bmp, (Int32)(cx - 12), (Int32)(cy - 12), 24, 24, icon.x, icon.y, icon.w, icon.h, BMP_ALLOWALPHA);
            }
            else
            {
                DrawSetTextCol(isSelected8 ? Vector(1.0) : Vector(0.85), COLOR_TRANS);
                DrawText("Center"_s, (Int32)cx, (Int32)(cy - 5), DRAWTEXT_HALIGN_CENTER);
            }
        }
        else
        {
            DrawSetTextCol(Vector(0.5, 0.52, 0.55), COLOR_TRANS);
            DrawText("Hub"_s, (Int32)cx, (Int32)(cy - 5), DRAWTEXT_HALIGN_CENTER);
        }
    }

    // Bottom hint
    DrawSetTextCol(Vector(0.52, 0.55, 0.60), COLOR_TRANS);
    DrawText("Click slot to inspect | Drag command here to assign"_s, (Int32)cx, h - 14, DRAWTEXT_HALIGN_CENTER);
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
                Redraw();
                return GeUserArea::Message(msg, result);
            }

            Int32 type = 0;
            void* object = nullptr;
            GetDragObject(msg, &type, &object);

            if (type == DRAGTYPE_COMMAND)
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
