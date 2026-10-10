#ifndef PIEMENU_EDITOR_AREA_H__
#define PIEMENU_EDITOR_AREA_H__

#include "c4d.h"
#include "piemenu_data.h"

namespace cinema
{

class PieMenuEditorArea : public GeUserArea
{
public:
    PieMenuEditorArea();
    virtual ~PieMenuEditorArea() = default;

    virtual Bool GetMinSize(Int32& w, Int32& h) override;
    virtual void DrawMsg(Int32 x1, Int32 y1, Int32 x2, Int32 y2, const BaseContainer& msg) override;
    virtual Bool InputEvent(const BaseContainer& msg) override;
    virtual Int32 Message(const BaseContainer& msg, BaseContainer& result) override;

    void SetActiveProfileIndex(Int32 index);
    Int32 GetActiveProfileIndex() const { return m_profileIndex; }

    void SetSelectedSlot(Int32 slot);
    Int32 GetSelectedSlot() const { return m_selectedSlot; }

    Int32 GetSlotUnderCoord(Int32 x, Int32 y) const;
    static Int32 ExtractCommandId(Int32 type, void* object, const BaseContainer& msg);
    static maxon::String ExtractSubmenuName(Int32 type, void* object, const BaseContainer& msg);

    Int32 GetDragSourceSlot() const { return m_dragSourceSlot; }

private:
    Int32 m_profileIndex = 0;
    Int32 m_selectedSlot = 0;
    Int32 m_dragHoverSlot = NOTOK;
    Int32 m_dragSourceSlot = NOTOK;
};

} // namespace cinema

#endif // PIEMENU_EDITOR_AREA_H__
