#include "c4d.h"
#include "piemenu_scenehook.h"
#include "piemenu_editor_dialog.h"
#include "piemenu_overlay_win.h"

namespace cinema
{

Bool PluginStart()
{
    PieMenuOverlay::Get().Init();

    if (!PieMenuSceneHook::Register())
        return false;

    if (!PieMenuEditorCommand::Register())
        return false;

    return true;
}

void PluginEnd()
{
    PieMenuOverlay::Get().Shutdown();
}

Bool PluginMessage(Int32 id, void* data)
{
    switch (id)
    {
        case C4DPL_INIT_SYS:
            if (!g_resource.Init())
                return false;
            return true;

        case C4DMSG_PRIORITY:
            return true;

        case C4DPL_BUILDMENU:
            break;
    }

    return false;
}

} // namespace cinema
