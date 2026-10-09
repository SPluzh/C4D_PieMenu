#include "piemenu_scenehook.h"
#include <cmath>
#include "c4d_general.h"
#include "windows_include.h"

namespace cinema
{

Bool PieMenuSceneHook::Register()
{
    return RegisterSceneHookPlugin(
        PLUGIN_ID_PIEMENU_SCENEHOOK,
        "Pie Menu Scene Hook"_s,
        PLUGINFLAG_SCENEHOOK_NOTDRAGGABLE,
        PieMenuSceneHook::Alloc,
        EXECUTIONPRIORITY_GENERATOR,
        0
    );
}

static Bool IsTriggerHeld()
{
    PieProfileManager& mgr = PieProfileManager::Get();
    if (mgr.m_useCtrlShift)
    {
#if defined(MAXON_TARGET_WINDOWS)
        SHORT ctrlState = GetAsyncKeyState(VK_CONTROL);
        SHORT shiftState = GetAsyncKeyState(VK_SHIFT);
        if ((ctrlState & 0x8000) && (shiftState & 0x8000))
            return true;
#endif
        BaseContainer keyState;
        if (GetInputState(BFM_INPUT_KEYBOARD, BFM_INPUT_QUALIFIER, keyState))
        {
            Int32 qual = keyState.GetInt32(BFM_INPUT_QUALIFIER);
            if ((qual & QSHIFT) && (qual & QCTRL))
                return true;
        }
        return false;
    }
    else if (mgr.m_triggerKey > 0)
    {
#if defined(MAXON_TARGET_WINDOWS)
        if (mgr.m_triggerKey >= 'A' && mgr.m_triggerKey <= 'Z')
        {
            if (GetAsyncKeyState(mgr.m_triggerKey) & 0x8000)
                return true;
        }
#endif
        BaseContainer keyState;
        if (GetInputState(BFM_INPUT_KEYBOARD, mgr.m_triggerKey, keyState))
        {
            if (keyState.GetInt32(BFM_INPUT_VALUE) != 0)
                return true;
        }
    }
    return false;
}

Bool PieMenuSceneHook::GetCursorInfo(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
{
    // BFM_CURSORINFO_REMOVE is sent by Cinema 4D when cursor info is being reset or redrawn.
    // Crucially, we must NOT deactivate m_active here!
    if (bc.GetId() == BFM_CURSORINFO_REMOVE)
        return true;

    m_lastMouseX = x;
    m_lastMouseY = y;

    PieProfileManager& mgr = PieProfileManager::Get();
    Bool triggered = IsTriggerHeld();

    if (triggered)
    {
        if (!m_active)
        {
            m_active = true;
            m_originX = x;
            m_originY = y;
            m_currentX = x;
            m_currentY = y;
            m_hoveredSlot = NOTOK;
            m_activeProfile = mgr.GetActiveProfileForDoc(doc);
            DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
        }
        else
        {
            if (x != m_currentX || y != m_currentY)
            {
                m_currentX = x;
                m_currentY = y;
                UpdateHover(x, y);
                DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
            }
        }

        bc.SetInt32(RESULT_CURSOR, MOUSE_POINT_HAND);

        if (m_activeProfile && m_hoveredSlot >= 0 && m_hoveredSlot < 9)
        {
            const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
            StatusSetText(FormatString("Pie Menu [@]: @ | Release Ctrl+Shift to execute"_s, m_activeProfile->name, slot.GetEffectiveTitle()));
        }
        else
        {
            StatusSetText(FormatString("Pie Menu [@]: Move cursor toward a slot to select"_s, m_activeProfile ? m_activeProfile->name : "Radial"_s));
        }

        return true;
    }
    else if (m_active && !triggered)
    {
        // Released! Commit selection
        CommitSelection();
        m_active = false;
        m_hoveredSlot = NOTOK;
        StatusSetText(""_s);
        DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
        return true;
    }

    return SceneHookData::GetCursorInfo(node, doc, bd, x, y, bc);
}

Bool PieMenuSceneHook::MouseInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
    if (m_active)
    {
        Int32 chn = msg.GetInt32(BFM_INPUT_CHANNEL);
        if (chn == BFM_INPUT_MOUSELEFT)
        {
            CommitSelection();
            m_active = false;
            m_hoveredSlot = NOTOK;
            StatusSetText(""_s);
            DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
            return true;
        }
        else if (chn == BFM_INPUT_MOUSERIGHT)
        {
            m_active = false;
            m_hoveredSlot = NOTOK;
            StatusSetText(""_s);
            DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
            return true;
        }
    }

    return SceneHookData::MouseInput(node, doc, bd, win, msg);
}

Bool PieMenuSceneHook::KeyboardInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
    Int32 key = msg.GetData(BFM_INPUT_CHANNEL).GetInt32();

    if (m_active)
    {
        if (key == KEY_ESC)
        {
            m_active = false;
            m_hoveredSlot = NOTOK;
            StatusSetText(""_s);
            DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
            return true;
        }

        if (!IsTriggerHeld())
        {
            CommitSelection();
            m_active = false;
            m_hoveredSlot = NOTOK;
            StatusSetText(""_s);
            DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
            return true;
        }

        // Consume keyboard events while active
        return true;
    }
    else
    {
        if (IsTriggerHeld())
        {
            m_active = true;
            m_originX = m_lastMouseX;
            m_originY = m_lastMouseY;
            m_currentX = m_lastMouseX;
            m_currentY = m_lastMouseY;
            m_hoveredSlot = NOTOK;
            m_activeProfile = PieProfileManager::Get().GetActiveProfileForDoc(doc);
            DrawViews(DRAWFLAGS::ONLY_ACTIVE_VIEW | DRAWFLAGS::NO_THREAD | DRAWFLAGS::NO_ANIMATION);
            return true;
        }
    }

    return SceneHookData::KeyboardInput(node, doc, bd, win, msg);
}

void PieMenuSceneHook::UpdateHover(Float curX, Float curY)
{
    if (!m_activeProfile)
    {
        m_hoveredSlot = NOTOK;
        return;
    }

    PieProfileManager& mgr = PieProfileManager::Get();
    Float dx = curX - m_originX;
    Float dy = curY - m_originY;
    Float dist = std::sqrt(dx * dx + dy * dy);

    if (dist < mgr.m_deadzone)
    {
        if (m_activeProfile->slots[8].enabled && m_activeProfile->slots[8].commandId > 0)
        {
            m_hoveredSlot = 8;
        }
        else
        {
            m_hoveredSlot = NOTOK;
        }
        return;
    }

    Float angleRad = std::atan2(dy, dx);
    Float deg = angleRad * (180.0 / PI);
    m_hoveredSlot = m_activeProfile->AngleToSlot(deg, dist, mgr.m_deadzone);
}

void PieMenuSceneHook::CommitSelection()
{
    if (!m_activeProfile || m_hoveredSlot == NOTOK)
        return;

    if (m_hoveredSlot >= 0 && m_hoveredSlot < 9)
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (slot.enabled && slot.commandId > 0)
        {
            CallCommand(slot.commandId);
        }
    }
}

Bool PieMenuSceneHook::Draw(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, SCENEHOOKDRAW flags)
{
    if (!bd || !m_active || !m_activeProfile)
        return true;

    // Draw in the standard drawing pass
    if ((flags & SCENEHOOKDRAW::DRAW_PASS) == SCENEHOOKDRAW::NONE)
        return true;

    // Fail-safe: if trigger is physically no longer held, commit & close
    if (!IsTriggerHeld())
    {
        CommitSelection();
        m_active = false;
        m_hoveredSlot = NOTOK;
        StatusSetText(""_s);
        return true;
    }

    DrawPieMenu(bd);
    return true;
}

void PieMenuSceneHook::DrawPieMenu(BaseDraw* bd)
{
    if (!bd || !m_active || !m_activeProfile)
        return;

    PieProfileManager& mgr = PieProfileManager::Get();

    bd->SetMatrix_Screen();
    bd->SetDrawParam(DRAW_PARAMETER_USE_Z, GeData(false));
    bd->SetDrawParam(DRAW_PARAMETER_SETZ, GeData(DRAW_Z_ALWAYS));
    bd->SetDrawParam(DRAW_PARAMETER_LINEWIDTH, GeData(1.0));

    Float ox = m_originX;
    Float oy = m_originY;
    Float curX = m_currentX;
    Float curY = m_currentY;

    auto drawScreenQuad = [&](Float x1, Float y1, Float x2, Float y2, const Vector& col)
    {
        Vector pts[4] = {
            Vector(x1, y1, 0.0),
            Vector(x2, y1, 0.0),
            Vector(x2, y2, 0.0),
            Vector(x1, y2, 0.0)
        };
        Vector cols[4] = { col, col, col, col };
        bd->DrawPolygon(pts, cols, true);
        bd->DrawArrayEnd();
    };

    auto drawScreenFrame = [&](Float x1, Float y1, Float x2, Float y2, const Vector& col, Float thickness)
    {
        bd->SetPen(col);
        bd->DrawLine2D(Vector(x1, y1, 0.0), Vector(x2, y1, 0.0));
        bd->DrawLine2D(Vector(x2, y1, 0.0), Vector(x2, y2, 0.0));
        bd->DrawLine2D(Vector(x2, y2, 0.0), Vector(x1, y2, 0.0));
        bd->DrawLine2D(Vector(x1, y2, 0.0), Vector(x1, y1, 0.0));
        if (thickness > 1.0)
        {
            bd->DrawLine2D(Vector(x1 - 1.0, y1 - 1.0, 0.0), Vector(x2 + 1.0, y1 - 1.0, 0.0));
            bd->DrawLine2D(Vector(x2 + 1.0, y1 - 1.0, 0.0), Vector(x2 + 1.0, y2 + 1.0, 0.0));
            bd->DrawLine2D(Vector(x2 + 1.0, y2 + 1.0, 0.0), Vector(x1 - 1.0, y2 + 1.0, 0.0));
            bd->DrawLine2D(Vector(x1 - 1.0, y2 + 1.0, 0.0), Vector(x1 - 1.0, y1 - 1.0, 0.0));
        }
    };

    auto drawScreenCircleFilled = [&](Float cx, Float cy, Float r, const Vector& col, Int32 segments)
    {
        for (Int32 s = 0; s < segments; ++s)
        {
            Float a0 = (Float(s) / Float(segments)) * 2.0 * PI;
            Float a1 = (Float(s + 1) / Float(segments)) * 2.0 * PI;
            Vector pts[3] = {
                Vector(cx, cy, 0.0),
                Vector(cx + std::cos(a0) * r, cy + std::sin(a0) * r, 0.0),
                Vector(cx + std::cos(a1) * r, cy + std::sin(a1) * r, 0.0)
            };
            Vector cols[3] = { col, col, col };
            bd->DrawPolygon(pts, cols, false);
        }
        bd->DrawArrayEnd();
    };

    // 1. Center hub (deadzone)
    const Float deadzoneRadius = mgr.m_deadzone;
    const PieSlot& slot8 = m_activeProfile->slots[8];
    Bool centerConfigured = (slot8.enabled && slot8.commandId > 0);
    Bool centerHovered = (m_hoveredSlot == 8);

    Vector hubBgCol = centerHovered ? (Vector(0.2, 0.25, 0.3) + slot8.accentColor * 0.2) : Vector(0.12, 0.13, 0.16);
    drawScreenCircleFilled(ox, oy, deadzoneRadius, hubBgCol, 24);

    Vector hubBorderCol = centerHovered ? slot8.accentColor : (centerConfigured ? slot8.accentColor * 0.7 : Vector(0.38, 0.42, 0.48));
    bd->SetPen(hubBorderCol);
    bd->DrawCircle2D((Int32)ox, (Int32)oy, deadzoneRadius);

    if (centerConfigured)
    {
        String cTitle = slot8.GetEffectiveTitle();
        Int32 cLen = cTitle.GetLength();
        bd->SetPen(centerHovered ? Vector(1.0) : Vector(0.85, 0.88, 0.9));
        bd->DrawHUDText((Int32)(ox - (cLen * 6.5) * 0.5), (Int32)(oy - 5.0), cTitle);
    }
    else
    {
        bd->SetPen(Vector(0.5, 0.55, 0.6));
        bd->DrawLine2D(Vector(ox - 4.0, oy, 0.0), Vector(ox + 4.0, oy, 0.0));
        bd->DrawLine2D(Vector(ox, oy - 4.0, 0.0), Vector(ox, oy + 4.0, 0.0));
    }

    // 2. Direction pointer line from center to cursor (when outside deadzone)
    Float vdx = curX - ox;
    Float vdy = curY - oy;
    Float dist = std::sqrt(vdx * vdx + vdy * vdy);
    if (dist > deadzoneRadius)
    {
        Vector pointerCol = (m_hoveredSlot != NOTOK && m_hoveredSlot >= 0 && m_hoveredSlot < 9) ?
            m_activeProfile->slots[m_hoveredSlot].accentColor : Vector(0.65, 0.65, 0.7);
        bd->SetPen(pointerCol);
        bd->DrawLine2D(Vector(ox, oy, 0.0), Vector(curX, curY, 0.0));
        drawScreenCircleFilled(curX, curY, 3.5, pointerCol, 12);
    }

    // 3. Context mode badge
    String badge = "["_s + m_activeProfile->name + "]"_s;
    Int32 bLen = badge.GetLength();
    bd->SetPen(Vector(0.5, 0.7, 0.9));
    bd->DrawHUDText((Int32)(ox - (bLen * 6.5) * 0.5), (Int32)(oy - mgr.m_radius - 30.0), badge);

    // 4. Render radial tool cards
    Int32 count = m_activeProfile->slotCount;
    if (count != 4 && count != 8) count = 8;

    const Float distCard = mgr.m_radius;
    const Float baseW = (count == 4) ? 120.0 : 105.0;
    const Float baseH = (count == 4) ? 30.0 : 26.0;

    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = m_activeProfile->slots[i];
        Bool isHovered = (m_hoveredSlot == i);
        Float angle = m_activeProfile->GetSlotAngle(i);

        Float cx = ox + std::cos(angle) * distCard;
        Float cy = oy + std::sin(angle) * distCard;

        Float hw = (isHovered ? baseW + 10.0 : baseW) * 0.5;
        Float hh = (isHovered ? baseH + 6.0 : baseH) * 0.5;

        Float x1 = cx - hw;
        Float x2 = cx + hw;
        Float y1 = cy - hh;
        Float y2 = cy + hh;

        Vector bgCol;
        if (!slot.enabled)
        {
            bgCol = Vector(0.06, 0.06, 0.07);
        }
        else if (isHovered)
        {
            bgCol = Vector(0.18, 0.22, 0.26) + slot.accentColor * 0.15;
        }
        else
        {
            bgCol = Vector(0.09, 0.10, 0.12);
        }

        drawScreenQuad(x1, y1, x2, y2, bgCol);

        Vector frameCol = isHovered ? slot.accentColor : (slot.enabled ? Vector(0.30, 0.32, 0.36) : Vector(0.16, 0.16, 0.18));
        drawScreenFrame(x1, y1, x2, y2, frameCol, isHovered ? 2.0 : 1.0);

        // Connector line from center hub to card
        Vector spokeCol = isHovered ? slot.accentColor * 0.75 : Vector(0.22, 0.24, 0.28);
        bd->SetPen(spokeCol);
        Float spokeStartX = ox + std::cos(angle) * (deadzoneRadius + 2.0);
        Float spokeStartY = oy + std::sin(angle) * (deadzoneRadius + 2.0);
        Float spokeEndX = cx - std::cos(angle) * (hw + 2.0);
        Float spokeEndY = cy - std::sin(angle) * (hh + 2.0);
        bd->DrawLine2D(Vector(spokeStartX, spokeStartY, 0.0), Vector(spokeEndX, spokeEndY, 0.0));

        // Label text
        String label = slot.GetEffectiveTitle();
        Int32 charCount = label.GetLength();
        Int32 textX = (Int32)(cx - (charCount * 6.5) * 0.5);
        Int32 textY = (Int32)(cy - 5.0);

        Vector textCol = isHovered ? Vector(1.0, 1.0, 1.0) : (slot.enabled ? Vector(0.82, 0.84, 0.88) : Vector(0.4, 0.4, 0.45));
        bd->SetPen(textCol);
        bd->DrawHUDText(textX, textY, label);
    }
}

} // namespace cinema
