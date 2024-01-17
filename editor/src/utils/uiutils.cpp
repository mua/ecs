#include "uiutils.h"

#include "imgui.h"

bool UIUtils::toggleButtonSet(std::vector<const char *> items, int *selected)
{
    bool ret = false;
    for (size_t i = 0; i < items.size(); i++)
    {
        ImGui::PushStyleColor(ImGuiCol_Button, *selected == i ? ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered)
                                                              : ImGui::GetStyleColorVec4(ImGuiCol_Button));

        if (ImGui::Button(items[i], ImVec2(24, 24)))
        {
            *selected = i;
            ret = true;
        }
        if (i < items.size() - 1)
            ImGui::SameLine();
        ImGui::PopStyleColor(1);
    }
    return ret;
}