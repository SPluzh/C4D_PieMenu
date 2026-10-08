#include "c4d.h"
#include "piemenu_scenehook.h"
#include "piemenu_editor_dialog.h"

namespace cinema
{

Bool PluginStart()
{
    if (!PieMenuSceneHook::Register())
        return false;

    if (!PieMenuEditorCommand::Register())
        return false;

    return true;
}

void PluginEnd()
{
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
