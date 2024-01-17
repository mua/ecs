#ifndef assets_h__
#define assets_h__

#include "utils/filenode.h"

struct AssetsView
{
    AssetsView();

    void gui();
    void assets();
    void dirTree(FileNode &fileNode, FileNode &root, std::string &selectedPath);
    void fileList(std::string path);

    FileNode dir;
    std::string selectedPath;
    std::string selectedFile;
};

#endif // assets_h__