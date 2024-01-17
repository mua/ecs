#include "views/registry.h"

#include "components/core.h"
#include "components/render.h"
#include "editor.h"
#include "engine.h"
#include "imgui.h"
#include "loguru.hpp"
#include "registry.h"

int selected = 0;

template <typename C> void guiComponent(C &component)
{
    ImGui::Text(typeid(component).name());
}

template <> void guiComponent(Info &component)
{
    ImGui::LabelText("Name", component.name.c_str());
    ImGui::Checkbox("Active", &component.active);
}

template <> void guiComponent(Ref &component)
{
    ImGui::LabelText("Path", component.path.c_str());
}

template <> void guiComponent(Relation &component)
{
    ImGui::LabelText("Parent",std::to_string(component.parent).c_str());
}

template <> void guiComponent(Renderable &component)
{
    ImGui::LabelText("Renderable Link", std::to_string(component.handle).c_str());
}

template <> void guiComponent(Geometry &component)
{
    ImGui::LabelText("Vertex Count", std::to_string(component.vertices.size()).c_str());
    ImGui::LabelText("Material Link", std::to_string(component.material).c_str());
}

template <> void guiComponent(Mesh &mesh)
{
    ImGui::BeginGroup();
    for (auto handle : mesh.geometries)
    {
        ImGui::LabelText("Geometry", std::to_string(handle).c_str());
    }
    ImGui::EndGroup();
}

template <> void guiComponent(Texture &component)
{
    ImGui::LabelText("Path", component.path.c_str());
    ImGui::LabelText("Size", glm::to_string(component.size).c_str());
    ImGui::LabelText("Byte Size", std::to_string(component.pixels.size()).c_str());
}

template <> void guiComponent(GLTexture &component)
{
    ImVec2 uv_min = ImVec2(0.0f, 1.0f);                 // Top-left
    ImVec2 uv_max = ImVec2(1.0f, 0.0f);                 // Lower-right
    ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
    ImVec4 border_col = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
    ImGui::Image(ImTextureID(component.textureName), ImVec2(128, 128), uv_min, uv_max, tint_col, border_col);
}

template <> void guiComponent(Material &mat)
{
    ImGui::LabelText("Type", mat.type == Material::PBR ? "PBR" : "Basic");
    ImGui::ColorEdit4("Diffuse Color", &mat.diffuseColor.x);
    ImGui::LabelText("Texture Count", std::to_string(mat.textures.size()).c_str());
}

template <> void guiComponent(RenderPassInstance &pass)
{
    ImGui::Columns(pass.attachments.size());
    for (int i = 0; i < pass.attachments.size(); i++)
    {
        ImGui::Text(RenderDescriptorTypeNames[pass.attachments[i].info.name - 1]);
        guiComponent(pass.attachments[i].texture);
        ImGui::NextColumn();
    }
    ImGui::Columns(1);
}

template <typename C> void guiComponentType(Handle entity)
{
    auto &r = Engine::instance()->registry;
    if (r.has<C>(entity))
    {
        auto &[comp] = r.getEntity<C>(entity);
        guiComponent<C>(comp);
        ImGui::Separator();
    }
}

void entityTree(EntityInfo info, std::map<Handle, EntityInfo> &entities);

void RegistryView::gui()
{
    auto &r = Engine::instance()->registry;
    auto &edState = r.get<EditorState>(Editor::instance()->editor);
    if (edState.selection.size())
    {
        selected = edState.selection[0];
    }
    auto s = ImGui::GetWindowHeight();
    ImGui::Begin("Entities");
    {
        int filter = 0;
        ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
        if (ImGui::BeginTabBar("Scopes", tab_bar_flags))
        {
            if (ImGui::BeginTabItem("Game"))
            {
                filter = 0;
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Editor"))
            {
                filter = 1;
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        ImGui::Separator();

        ImGuiTableFlags flags = ImGuiTableFlags_BordersOuter | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY |
                                ImGuiTableFlags_ScrollFreezeTopRow | ImGuiTableFlags_Resizable;
        if (ImGui::BeginTable("##table1", 10, flags))
        {
            auto flags = ImGuiTableColumnFlags_WidthStretch;
            ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_NoHide);
            ImGui::TableSetupColumn("ID", flags, ImGui::GetFontSize() * 6);
            ImGui::TableSetupColumn("c0", flags, ImGui::GetFontSize() * 10);
            ImGui::TableSetupColumn("c1", flags, ImGui::GetFontSize() * 6);
            ImGui::TableSetupColumn("c2", flags, ImGui::GetFontSize() * 6);
            ImGui::TableSetupColumn("c3", flags, ImGui::GetFontSize() * 6);
            ImGui::TableSetupColumn("c4", flags, ImGui::GetFontSize() * 6);
            ImGui::TableSetupColumn("c5", flags, ImGui::GetFontSize() * 6);
            ImGui::TableSetupColumn("c6", flags, ImGui::GetFontSize() * 6);
            ImGui::TableSetupColumn("c7", flags, ImGui::GetFontSize() * 6);
            ImGui::TableAutoHeaders();

            std::map<Handle, EntityInfo> entities = r.entities();
            for (auto entity : entities)
            {
                auto rel = r.getPtr<Relation>(entity.first);
                if (rel && rel->parent)
                    continue;
                auto inf = r.getPtr<Info>(entity.first);
                if (inf && inf->scope != filter)
                {
                    continue;
                }

                entityTree(entity.second, entities);
            }

            ImGui::EndTable();
        }
    }
    ImGui::End();
    ImGui::Begin("Components");
    {
        guiComponentType<Info>(selected);
        guiComponentType<Relation>(selected);
        guiComponentType<Renderable>(selected);
        guiComponentType<Geometry>(selected);
        guiComponentType<Material>(selected);
        guiComponentType<Texture>(selected);
        guiComponentType<GLTexture>(selected);
        guiComponentType<RenderPassInstance>(selected);
        guiComponentType<Mesh>(selected);
        guiComponentType<Ref>(selected);
    }
    ImGui::End();
}

void entityTree(EntityInfo info, std::map<Handle, EntityInfo> &entities)
{
    auto &r = Engine::instance()->registry;
    Handle entity = info.handle;
    bool isSelected = selected == entity;
    auto &edState = r.get<EditorState>(Editor::instance()->editor);
    ImGui::TableNextRow();
    if (isSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    }
    else
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_Text));
        ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, ImGui::GetStyleColorVec4(ImGuiCol_TableRowBg));
    }

    auto &[rel] = r.getEntity<Relation>(entity);
    Info eif;
    std::string name = "Entity";
    if (r.get(entity, eif))
    {
        name = eif.name;
    }
    const bool hasChildren = &rel && rel.children.size();

    auto flags = hasChildren ? ImGuiTreeNodeFlags_DefaultOpen
                             : ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet |
                                   ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanFullWidth;

    flags |= isSelected ? ImGuiTreeNodeFlags_Selected : 0;
    flags |= ImGuiTreeNodeFlags_SpanFullWidth;

    bool active = false;
    auto idx = std::to_string(entity);
    bool open = ImGui::TreeNodeEx(name.c_str(), flags);
    if (ImGui::IsItemHovered())
    {
        active = true;
    }
    ImGui::TableNextCell();
    ImGui::TextUnformatted(idx.c_str());
//          if (ImGui::IsItemClicked())
//          {
//              //LOG_F(INFO, "Clicked name");
//              edState.selection = {entity};
//          }
    ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered));
    for (auto name : info.components)
    {
        if (name == "Relation")
            continue;
        ImGui::TableNextCell();
        ImGui::Selectable(name.c_str(), active,
                          ImGuiSelectableFlags_SpanAllColumns | +ImGuiSelectableFlags_AllowItemOverlap);
        if (ImGui::IsItemClicked())
        {
            // LOG_F(INFO, "Clicked name");   

            edState.selection = {entity};
        }
    }
    ImGui::PopStyleColor(3);
    if (open && hasChildren)
    {
        auto [rel] = r.getEntity<Relation>(entity);
        for (auto child : rel.children)
            entityTree(entities[child], entities);
        ImGui::TreePop();
    }
}
