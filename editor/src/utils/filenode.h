#ifndef filenode_h__
#define filenode_h__
#include <string>
#include <map>
#include <filesystem>
#include "utils.h"


struct FileNode
{
	std::string path;
	std::string nodePath;
	std::string name;
	std::string ext;
	bool isDir=false;
	std::map<std::string, FileNode*> children;

	FileNode();

	FileNode(std::string path);

	FileNode(std::filesystem::directory_entry& entry, std::string nodePath);

	void refresh();

	void set(std::filesystem::directory_entry& entry);

	FileNode* get(std::string nodePath);
};
#endif // filenode_h__
