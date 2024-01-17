#ifndef editor_h__
#define editor_h__

#include "types.h"
#include "systems/uisys.h"
#include "imgui.h"
#include "storage.h"


struct RegistryView;
struct AssetsView;
struct Camera;
struct ModifiersView;
class Engine;
struct GameView;

struct EditorState
{
    vec2 mouse;
    Ray mouseRay;
    bool mouseButtons[3];
    bool mouseTracked = false;
    std::vector<Handle> selection;
};
REGISTER_COMPONENT(EditorState)

struct Viewport
{
    std::string name;
};
REGISTER_COMPONENT(Viewport)

struct Game
{
    typedef void (*InitGame)(Engine *engine);
    typedef void (*DestroyGame)(void);
    typedef void (*StartGame)(void);
    typedef void (*PauseGame)(void);
    typedef void (*UpdateGame)(void);
    typedef void (*StopGame)(void);
    typedef void (*ResizeGame)(int width, int height);
    typedef void* (*TextureHandle)(void);

    std::wstring libraryPath;
    uint64_t modified;
    uint64_t module;

    InitGame initGame=0;
    DestroyGame destroyGame = 0;
    UpdateGame updateGame = 0;
    StartGame startGame = 0;
    PauseGame pauseGame = 0;
    StopGame stopGame = 0;
    TextureHandle textureHandle = 0;
    ResizeGame resizeGame = 0;


    Game();
    void load();
};

class Editor : public UISys
{
    AssetsView *assets;
    RegistryView *registry;  
    ModifiersView *modifiers;
    GameView *gameView;
    Game game;

    static Editor *_instance;
    
    void createViewport(std::string name);
  public:
    Handle editor;


    void start() override;
    void process() override;

    void mainWindow();

    void menu();

    void toolbar();

    ImVec2 vpPos(Camera &camera, mat4 model, ImVec2 wpos, ImVec2 size, vec3 pos);

    void viewports();    

    static Editor* instance();    
};
#endif // editor_h__
