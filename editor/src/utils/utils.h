#include <string>
#include <vector>

namespace EditorUtils
{
	std::string openFileName(std::string ext);
	std::string saveFileName(std::string ext);
	std::vector<std::string> splitString(const std::string& s, char delimiter);
}