#ifndef Application_h__
#define Application_h__

#include <glm/glm.hpp>
#include "json.hpp"

#include <fstream>

struct GLFWwindow;

using json = nlohmann::json;

class Application
{
private:
    /* data */
    static Application* _instance;
public:
    glm::vec4 clearColor = glm::vec4(0.45f, 0.55f, 0.60f, 1.00f);
    Application(/* args */);
    ~Application();
    void run();

	json settings;
	template <class T> void saveSetting(std::string name, T value)
    {
        settings[name] = value;
        std::ofstream fs("settings.json");
        fs << settings;
    }

    template <class T> T loadSetting(std::string name, T def = T())
    {
        if (settings.is_null())
        {
            std::ifstream fs("settings.json");

            if (!fs.fail())
                fs >> settings;
        }
        if (settings.is_null())
        {
            return def;
        }
        return settings[name].is_null() ? def : settings[name].get<T>();
    }

    static void onWindowResized(GLFWwindow *window, int width, int height);
    static Application *instance()
    {
        return _instance;
    }
};
#endif // Application_h__
