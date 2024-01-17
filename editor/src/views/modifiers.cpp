#include "modifiers.h"

#include "modifiers/selection.h"
#include "modifiers/transform.h"

#include "engine.h"
#include "editor.h"
#include "IconsFontAwesome.h"
#include "utils/uiutils.h"

 ModifiersView::ModifiersView()
{
     modifiers.push_back(new SelectionModifier());
     modifiers.push_back(new TranslationModifier());
     modifiers.push_back(new RotationModifier());     
     modifiers.push_back(new ScalingModifier());     
 }

void ModifiersView::gui()
{
    Engine::instance()->registry.each<Viewport>([&](Handle entity, Viewport &vp) {
        for (int i=modifiers.size()-1; i>-1; i--)
        {
            auto mod = modifiers[i];
            if (mod->enabled && mod->isAvailable() && mod->process(entity))
            {
                // break;
            }
        }
    });
}

void ModifiersView::toolbar()
{
    static int selected = 0;
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3.0f, 5.0f));
    UIUtils::toggleButtonSet({ICON_FA_MOUSE_POINTER, ICON_FA_ARROWS, ICON_FA_REFRESH, ICON_FA_EXPAND}, &selected);

    for (int i=1; i<modifiers.size(); i++)
    {
        modifiers[i]->enabled = selected == i;
    }
    
    ImGui::PopStyleVar(2);
}
