#include "assets.h"
#include "utils/filenode.h"

#include "imgui.h"

 AssetsView::AssetsView()
{
     dir = FileNode(".");
}

void AssetsView::gui()
{
    assets();    
}

void AssetsView::assets()
{
    if (ImGui::Begin("Assets"))
    {
        ImGui::BeginChild("left pane", ImVec2(200, 0), true);
        {
            dirTree(dir, dir, selectedPath);
        }
        ImGui::EndChild();

        ImGui::SameLine();
        ImGui::BeginChild("right pane", ImVec2(0, 0), true);
        {
            fileList(selectedPath);
        }
        ImGui::EndChild();
    }
    ImGui::End();
}

void AssetsView::dirTree(FileNode &fileNode, FileNode &root, std::string &selectedPath)
{
    ImGuiTreeNodeFlags nodeFlags = 0;
    nodeFlags |= ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    nodeFlags |= ImGuiTreeNodeFlags_OpenOnArrow;
    nodeFlags |= selectedPath == fileNode.nodePath ? ImGuiTreeNodeFlags_Selected : 0;
    nodeFlags |= fileNode.isDir ? 0 : ImGuiTreeNodeFlags_Leaf;
    nodeFlags |= &fileNode == &root ? ImGuiTreeNodeFlags_DefaultOpen : 0;

    ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, ImGui::GetFontSize() * 1);
    auto open = (ImGui::TreeNodeEx(fileNode.name.c_str(), nodeFlags));
    if (ImGui::IsItemClicked())
    {
        // selectDir(fileNode->nodePath);
        selectedPath = fileNode.nodePath;
        //		assert(directory->node(selectedPath).path == fileNode->path);
    }
    if (open)
    {
        for (auto &[name, entry] : fileNode.children)
        {
            if (entry->isDir)
            {
                dirTree(*entry, root, selectedPath);
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopStyleVar();
}


void AssetsView::fileList(std::string path)
{
    static char draggedFilePath[1024];
    ImGui::Columns(3, "mycolumns3", false);
    ImGui::Separator();
    auto dirNode = dir.get(path);

    for (auto &[key, entry] : dirNode->children)
    {
        bool isSelected = selectedFile == entry->path;
        if (entry->ext == ".json" && !isSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.45f, 0.60f, 1, 1));
        }
        else if (entry->isDir && !isSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.000f, 1.000f, 0.000f, 0.900f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_Text]);
        }
        if (ImGui::Selectable(entry->name.c_str(), &isSelected, ImGuiSelectableFlags_AllowDoubleClick))
        {
            selectedFile = entry->path;
            if (ImGui::IsMouseDoubleClicked(0))
            {
                if (entry->isDir)
                {
                    selectedPath = entry->nodePath;
                }
                else
                {
                    //EditorApp::instance().openFile(std::filesystem::absolute(entry->path).string());
                }
            }
        }
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            strcpy(draggedFilePath, entry->path.c_str());
            ImGui::SetDragDropPayload("FileName", draggedFilePath, sizeof(draggedFilePath));
            ImGui::Text(entry->path.c_str());
            ImGui::EndDragDropSource();
        }
        ImGui::PopStyleColor();
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::Selectable("Open in json editor"))
            {
                system(("code " + entry->path).c_str());
            }
            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
        ImGui::NextColumn();
    }
}