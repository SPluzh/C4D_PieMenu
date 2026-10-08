#ifndef PIEMENU_SCENEHOOK_H__
#define PIEMENU_SCENEHOOK_H__

#include "c4d.h"
#include "piemenu_ids.h"
#include "piemenu_data.h"

namespace cinema
{

class PieMenuSceneHook : public SceneHookData
{
public:
    static NodeData* Alloc() { return NewObjClear(PieMenuSceneHook); }

    virtual Bool MouseInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg) override;
    virtual Bool KeyboardInput(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, EditorWindow* win, const BaseContainer& msg) override;
    virtual Bool GetCursorInfo(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, Float x, Float y, BaseContainer& bc) override;
    virtual Bool Draw(BaseSceneHook* node, BaseDocument* doc, BaseDraw* bd, BaseDrawHelp* bh, BaseThread* bt, SCENEHOOKDRAW flags) override;

    static Bool Register();

private:
    void DrawPieMenu(BaseDraw* bd);
    void UpdateHover(Float curX, Float curY);
    void CommitSelection();

    Bool        m_active = false;
    Float       m_originX = 0.0;
    Float       m_originY = 0.0;
    Float       m_currentX = 0.0;
    Float       m_currentY = 0.0;
    Int32       m_hoveredSlot = NOTOK;
    PieProfile* m_activeProfile = nullptr;
};

} // namespace cinema

#endif // PIEMENU_SCENEHOOK_H__
