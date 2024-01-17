#include "filenode.h"

 FileNode::FileNode(std::filesystem::directory_entry &entry, std::string nodePath) : nodePath(nodePath)
{
    set(entry);
}

 FileNode::FileNode(std::string path) : path(path)
{
    refresh();
}

 FileNode::FileNode()
{
}

void FileNode::refresh()
{
    auto entry = std::filesystem::directory_entry(std::filesystem::path(path));
    children.clear();
    set(entry);
}

void FileNode::set(std::filesystem::directory_entry &entry)
{
    isDir = entry.is_directory();
    path = entry.path().string();
    ext = entry.path().extension().string();
    name = entry.path().filename().string();
    if (isDir)
    {
        std::vector<FileNode> nodes;
        for (auto entry : std::filesystem::directory_iterator(path))
        {
            auto cname = entry.path().filename().string();
            children[cname] = new FileNode(entry, !nodePath.empty() ? nodePath + "/" + cname : cname);
        }
    }
}

FileNode *FileNode::get(std::string nodePath)
{
    auto names = EditorUtils::splitString(nodePath, '/');
    FileNode *entry = this;
    for (auto name : names)
    {
        entry = entry->children[name];
    }
    return entry;
}
