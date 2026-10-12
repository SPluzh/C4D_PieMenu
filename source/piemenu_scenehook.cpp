#include "piemenu_scenehook.h"
#include "piemenu_overlay_win.h"
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

Bool PieMenuSceneHook::GetCursorInfo(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, Float x, Float y, BaseContainer& bc)
{
    if (bc.GetId() == BFM_CURSORINFO_REMOVE)
        return true;

    m_lastMouseX = x;
    m_lastMouseY = y;

    // Check trigger and delegate to transparent overlay
    PieMenuOverlay::Get().SyncKeysFromAsync();
    PieMenuOverlay::Get().CheckTrigger();
    if (PieMenuOverlay::Get().IsActive())
    {
        bc.SetInt32(RESULT_CURSOR, MOUSE_POINT_HAND);
        return true;
    }

    return SceneHookData::GetCursorInfo(node, doc, bd, x, y, bc);
}

Bool PieMenuSceneHook::MouseInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
    if (PieMenuOverlay::Get().IsActive())
        return true;

    return SceneHookData::MouseInput(node, doc, bd, win, msg);
}

Bool PieMenuSceneHook::KeyboardInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg)
{
    PieMenuOverlay::Get().SyncKeysFromAsync();
    PieMenuOverlay::Get().CheckTrigger();
    if (PieMenuOverlay::Get().IsActive())
        return true;

    return SceneHookData::KeyboardInput(node, doc, bd, win, msg);
}

void PieMenuSceneHook::UpdateHover(Float curX, Float curY)
{
    if (!m_activeProfile)
    {
        m_hoveredSlot = NOTOK;
        return;
    }

    Float dx = curX - m_originX;
    Float dy = curY - m_originY;
    Float dist = std::sqrt(dx * dx + dy * dy);

    const Float deadzone = (m_activeProfile->deadzone > 0.0) ? m_activeProfile->deadzone : 26.0;

    if (dist < deadzone)
    {
        if (m_activeProfile->slots[CENTER_SLOT_INDEX].enabled && (m_activeProfile->slots[CENTER_SLOT_INDEX].commandId > 0 || m_activeProfile->slots[CENTER_SLOT_INDEX].IsSubMenu()))
        {
            m_hoveredSlot = CENTER_SLOT_INDEX;
        }
        else
        {
            m_hoveredSlot = NOTOK;
        }
        return;
    }

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

void PieMenuSceneHook::CommitSelection()
{
    if (!m_activeProfile || m_hoveredSlot == NOTOK)
        return;

    if (m_hoveredSlot >= 0 && (m_hoveredSlot < m_activeProfile->slotCount || m_hoveredSlot == CENTER_SLOT_INDEX))
    {
        const PieSlot& slot = m_activeProfile->slots[m_hoveredSlot];
        if (slot.enabled)
        {
            if (slot.IsSubMenu())
            {
                PieProfile* sub = PieProfileManager::Get().FindProfileByName(slot.subMenuName);
                if (sub)
                {
                    m_activeProfile = sub;
                    m_hoveredSlot = NOTOK;
                }
            }
            else if (slot.commandId > 0 || slot.HasModifierCommands())
            {
#if defined(MAXON_TARGET_WINDOWS)
                bool shiftHeld = ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0);
                bool ctrlHeld = ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
                bool altHeld = ((GetAsyncKeyState(VK_MENU) & 0x8000) != 0);
#else
                bool shiftHeld = false, ctrlHeld = false, altHeld = false;
#endif
                if (m_activeProfile && m_activeProfile->hotkey.enabled)
                {
                    if (m_activeProfile->hotkey.shift) shiftHeld = false;
                    if (m_activeProfile->hotkey.ctrl)  ctrlHeld  = false;
                    if (m_activeProfile->hotkey.alt)   altHeld   = false;
                }
                Int32 cmd = slot.GetCommandForModifiers(shiftHeld, ctrlHeld, altHeld);
                if (cmd > 0)
                {
                    CallCommand(cmd);
                }
            }
        }
    }
}

Bool PieMenuSceneHook::Draw(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, SCENEHOOKDRAW flags)
{
    if (PieMenuOverlay::Get().IsActive())
        return true;

    return true;
}

void PieMenuSceneHook::DrawPieMenu(BaseDraw* bd)
{
    if (!bd || !m_active || !m_activeProfile)
        return;

    bd->SetMatrix_Screen();
    bd->SetDrawParam(DRAW_PARAMETER_USE_Z, GeData(false));
    bd->SetDrawParam(DRAW_PARAMETER_SETZ, GeData(DRAW_Z_ALWAYS));
    bd->SetDrawParam(DRAW_PARAMETER_LINEWIDTH, GeData(1.0));

    Float ox = m_originX;
    Float oy = m_originY;
    Float curX = m_currentX;
    Float curY = m_currentY;

    // Lambda: draw filled annular ring sector (donut slice) using triangles
    auto drawAnnularSector = [&](Float cx, Float cy, Float rIn, Float rOut, Float aStart, Float aEnd, const Vector& fillColor, Int32 steps = 14)
    {
        for (Int32 s = 0; s < steps; ++s)
        {
            Float a0 = aStart + (Float(s) / Float(steps)) * (aEnd - aStart);
            Float a1 = aStart + (Float(s + 1) / Float(steps)) * (aEnd - aStart);

            Float c0 = std::cos(a0), s0 = std::sin(a0);
            Float c1 = std::cos(a1), s1 = std::sin(a1);

            Vector pIn0(cx + c0 * rIn, cy + s0 * rIn, 0.0);
            Vector pOut0(cx + c0 * rOut, cy + s0 * rOut, 0.0);
            Vector pOut1(cx + c1 * rOut, cy + s1 * rOut, 0.0);
            Vector pIn1(cx + c1 * rIn, cy + s1 * rIn, 0.0);

            Vector tri1[3] = { pIn0, pOut0, pOut1 };
            Vector triCol1[3] = { fillColor, fillColor, fillColor };
            bd->DrawPolygon(tri1, triCol1, false);

            Vector tri2[3] = { pIn0, pOut1, pIn1 };
            Vector triCol2[3] = { fillColor, fillColor, fillColor };
            bd->DrawPolygon(tri2, triCol2, false);
        }
        bd->DrawArrayEnd();
    };

    // Lambda: draw contour lines of an annular sector
    auto drawAnnularSectorBorder = [&](Float cx, Float cy, Float rIn, Float rOut, Float aStart, Float aEnd, const Vector& borderColor, Float lineWidth, Int32 steps = 14)
    {
        bd->SetDrawParam(DRAW_PARAMETER_LINEWIDTH, GeData(lineWidth));
        bd->SetPen(borderColor);

        // Outer arc
        for (Int32 s = 0; s < steps; ++s)
        {
            Float a0 = aStart + (Float(s) / Float(steps)) * (aEnd - aStart);
            Float a1 = aStart + (Float(s + 1) / Float(steps)) * (aEnd - aStart);
            bd->DrawLine2D(Vector(cx + std::cos(a0) * rOut, cy + std::sin(a0) * rOut, 0.0),
                           Vector(cx + std::cos(a1) * rOut, cy + std::sin(a1) * rOut, 0.0));
        }

        // Inner arc
        for (Int32 s = 0; s < steps; ++s)
        {
            Float a0 = aStart + (Float(s) / Float(steps)) * (aEnd - aStart);
            Float a1 = aStart + (Float(s + 1) / Float(steps)) * (aEnd - aStart);
            bd->DrawLine2D(Vector(cx + std::cos(a0) * rIn, cy + std::sin(a0) * rIn, 0.0),
                           Vector(cx + std::cos(a1) * rIn, cy + std::sin(a1) * rIn, 0.0));
        }

        // Radial dividing edges
        bd->DrawLine2D(Vector(cx + std::cos(aStart) * rIn, cy + std::sin(aStart) * rIn, 0.0),
                       Vector(cx + std::cos(aStart) * rOut, cy + std::sin(aStart) * rOut, 0.0));
        bd->DrawLine2D(Vector(cx + std::cos(aEnd) * rIn, cy + std::sin(aEnd) * rIn, 0.0),
                       Vector(cx + std::cos(aEnd) * rOut, cy + std::sin(aEnd) * rOut, 0.0));

        bd->SetDrawParam(DRAW_PARAMETER_LINEWIDTH, GeData(1.0));
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

    // Radii & spacing (proportional scaling if user customized radius)
    Float radiusScale = (m_activeProfile->radius > 20.0) ? (m_activeProfile->radius / 85.0) : 1.0;
    const Float deadzoneRadius = (m_activeProfile->deadzone > 0.0) ? m_activeProfile->deadzone : 26.0;
    const Float baseInnerR = 36.0 * radiusScale;
    const Float baseOuterR = 112.0 * radiusScale;
    const Float hoverInnerR = 33.0 * radiusScale;
    const Float hoverOuterR = 125.0 * radiusScale; // Pops out by 13px on hover

    Int32 count = m_activeProfile->slotCount;
    if (count < MIN_SLOT_COUNT) count = MIN_SLOT_COUNT;
    if (count > MAX_SLOT_COUNT) count = MAX_SLOT_COUNT;

    const Float deg2rad = PI / 180.0;
    const Float sliceDeg = 360.0 / Float(count);
    const Float gapDeg = (count <= 4) ? 2.2 : ((count <= 8) ? 1.8 : ((count <= 14) ? 1.2 : 0.8));
    const Float gapRad = (gapDeg * deg2rad) * 0.5;
    const Int32 steps = (count <= 4) ? 18 : ((count <= 8) ? 14 : ((count <= 14) ? 10 : 7));

    // 1. Draw circular segments (Fills and active indicators)
    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = m_activeProfile->slots[i];
        if (!slot.enabled)
            continue;

        Bool isHovered = (m_hoveredSlot == i);

        Float rIn = isHovered ? hoverInnerR : baseInnerR;
        Float rOut = isHovered ? hoverOuterR : baseOuterR;

        Float startDeg = 270.0 - sliceDeg * 0.5 + Float(i) * sliceDeg;
        Float aStart = startDeg * deg2rad + gapRad;
        Float aEnd = (startDeg + sliceDeg) * deg2rad - gapRad;

        Bool hasAction = (slot.commandId > 0 || slot.IsSubMenu());
        Vector bgCol;
        if (isHovered)
        {
            bgCol = Vector(0.14, 0.16, 0.20) + slot.accentColor * 0.22;
        }
        else if (hasAction)
        {
            bgCol = Vector(0.10, 0.11, 0.14) + slot.accentColor * 0.06;
        }
        else
        {
            bgCol = Vector(0.09, 0.10, 0.13);
        }

        // Draw sector body
        drawAnnularSector(ox, oy, rIn, rOut, aStart, aEnd, bgCol, steps);

        // If hovered: draw illuminated inner arc strip
        if (isHovered)
        {
            Vector activeStripCol = slot.accentColor;
            drawAnnularSector(ox, oy, rIn, rIn + 4.5 * radiusScale, aStart, aEnd, activeStripCol, steps);
        }
        else if (hasAction)
        {
            Vector activeStripCol = slot.accentColor * 0.6;
            drawAnnularSector(ox, oy, rIn, rIn + 3.0 * radiusScale, aStart, aEnd, activeStripCol, steps);
        }
    }

    // 2. Draw Borders around segments
    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = m_activeProfile->slots[i];
        if (!slot.enabled)
            continue;

        Bool isHovered = (m_hoveredSlot == i);

        Float rIn = isHovered ? hoverInnerR : baseInnerR;
        Float rOut = isHovered ? hoverOuterR : baseOuterR;

        Float startDeg = 270.0 - sliceDeg * 0.5 + Float(i) * sliceDeg;
        Float aStart = startDeg * deg2rad + gapRad;
        Float aEnd = (startDeg + sliceDeg) * deg2rad - gapRad;

        Bool hasAction = (slot.commandId > 0 || slot.IsSubMenu());
        Vector borderCol;
        Float borderWidth = 1.0;
        if (isHovered)
        {
            borderCol = slot.accentColor;
            borderWidth = 2.0;
        }
        else if (hasAction)
        {
            borderCol = slot.accentColor * 0.65;
            borderWidth = 1.2;
        }
        else
        {
            borderCol = Vector(0.24, 0.27, 0.31);
            borderWidth = 1.0;
        }

        drawAnnularSectorBorder(ox, oy, rIn, rOut, aStart, aEnd, borderCol, borderWidth, steps);
    }

    // 3. Draw Labels inside each segment
    for (Int32 i = 0; i < count; ++i)
    {
        const PieSlot& slot = m_activeProfile->slots[i];
        if (!slot.enabled)
            continue;

        Bool isHovered = (m_hoveredSlot == i);

        Float rIn = isHovered ? hoverInnerR : baseInnerR;
        Float rOut = isHovered ? hoverOuterR : baseOuterR;
        Float rText = (rIn + rOut) * 0.5;

        Float midDeg = 270.0 + Float(i) * sliceDeg;
        Float midRad = midDeg * deg2rad;

        Float tx = ox + std::cos(midRad) * rText;
        Float ty = oy + std::sin(midRad) * rText;

        String labelText = slot.GetEffectiveTitle();
        if (slot.IsSubMenu())
            labelText += " >"_s;
        Int32 charCount = labelText.GetLength();
        Int32 textX = (Int32)(tx - (charCount * 6.5) * 0.5);
        Int32 textY = (Int32)(ty - 5.0);

        Bool hasAction = (slot.commandId > 0 || slot.IsSubMenu());
        Vector textCol;
        if (isHovered)
        {
            textCol = Vector(1.0, 1.0, 1.0);
        }
        else if (hasAction)
        {
            textCol = slot.accentColor;
        }
        else
        {
            textCol = Vector(0.78, 0.82, 0.88);
        }

        bd->SetPen(textCol);
        bd->DrawHUDText(textX, textY, labelText);
    }

    // 4. Center hub (CENTER_SLOT_INDEX in Pie Menu / deadzone)
    const PieSlot& slot8 = m_activeProfile->slots[CENTER_SLOT_INDEX];
    Bool centerConfigured = (slot8.enabled && (slot8.commandId > 0 || slot8.IsSubMenu()));
    Bool centerHovered = (m_hoveredSlot == CENTER_SLOT_INDEX);

    Vector hubBgCol;
    if (centerHovered)
        hubBgCol = Vector(0.14, 0.16, 0.20) + slot8.accentColor * 0.22;
    else if (centerConfigured)
        hubBgCol = Vector(0.10, 0.11, 0.14) + slot8.accentColor * 0.08;
    else
        hubBgCol = Vector(0.10, 0.11, 0.14);

    drawScreenCircleFilled(ox, oy, deadzoneRadius, hubBgCol, 28);

    Vector hubBorderCol;
    if (centerHovered)
        hubBorderCol = slot8.accentColor;
    else if (m_hoveredSlot != NOTOK && m_hoveredSlot >= 0 && m_hoveredSlot < count)
        hubBorderCol = m_activeProfile->slots[m_hoveredSlot].accentColor * 0.8;
    else if (centerConfigured)
        hubBorderCol = slot8.accentColor * 0.65;
    else
        hubBorderCol = Vector(0.32, 0.35, 0.40);

    bd->SetPen(hubBorderCol);
    bd->DrawCircle2D((Int32)ox, (Int32)oy, deadzoneRadius);

    if (centerConfigured)
    {
        String cTitle = slot8.GetEffectiveTitle();
        if (slot8.IsSubMenu())
            cTitle += " >"_s;
        Int32 cLen = cTitle.GetLength();
        bd->SetPen(centerHovered ? Vector(1.0, 1.0, 1.0) : slot8.accentColor);
        bd->DrawHUDText((Int32)(ox - (cLen * 6.5) * 0.5), (Int32)(oy - 5.0), cTitle);
    }
    else
    {
        // Crosshair at center
        bd->SetPen(Vector(0.50, 0.54, 0.60));
        bd->DrawLine2D(Vector(ox - 3.5, oy, 0.0), Vector(ox + 3.5, oy, 0.0));
        bd->DrawLine2D(Vector(ox, oy - 3.5, 0.0), Vector(ox, oy + 3.5, 0.0));
    }

    // 5. Direction pointer line from center to cursor (when outside deadzone)
    Float vdx = curX - ox;
    Float vdy = curY - oy;
    Float dist = std::sqrt(vdx * vdx + vdy * vdy);
    if (dist > deadzoneRadius)
    {
        Vector pointerCol = (m_hoveredSlot != NOTOK && m_hoveredSlot >= 0 && m_hoveredSlot < count) ?
            m_activeProfile->slots[m_hoveredSlot].accentColor :
            (centerHovered ? slot8.accentColor : Vector(0.65, 0.65, 0.70));
        bd->SetPen(pointerCol);
        bd->DrawLine2D(Vector(ox, oy, 0.0), Vector(curX, curY, 0.0));

        // Cursor handle dot
        drawScreenCircleFilled(curX, curY, 3.5, pointerCol, 12);
    }

    // 6. Context mode badge
    if (m_activeProfile->showTitle && m_activeProfile->name.IsPopulated())
    {
        String badge = "["_s + m_activeProfile->name + "]"_s;
        Int32 bLen = badge.GetLength();
        bd->SetPen(Vector(0.50, 0.70, 0.90));
        bd->DrawHUDText((Int32)(ox - (bLen * 6.5) * 0.5), (Int32)(oy - hoverOuterR - 16.0), badge);
    }
}


} // namespace cinema
