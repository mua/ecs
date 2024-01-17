#include "editor.h"

#include "views/assets.h"
#include "views/registry.h"

#include "components/render.h"
#include "components/transform.h"

#include "systems/rendersys.h"

#include "engine.h"
#include "loguru.hpp"
#include "views/modifiers.h"

#include "IconsFontAwesome.h"
#include "components/core.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <chrono>
#include <thread>
#include <fstream>

#include "views/game.h"

Editor *Editor::_instance = nullptr;

void Editor::start()
{
    UISys::start();
    Editor::_instance = this;

    EditorState editorState;
    editor = r->createEntity(Info{"editorState", 1}, editorState);

    assets = new AssetsView();
    registry = new RegistryView();
    modifiers = new ModifiersView();
    gameView = new GameView();
    gameView->game = &game;

    createViewport("first");
    createViewport("second");
}

void Editor::process()
{
    r->each<Viewport>([&](Handle entity, Viewport& vp) {
        auto &canvasGeo = r->get<Geometry>(entity);
        //canvasGeo.layer = 2;
        canvasGeo.vertices.clear();
        canvasGeo.indices.clear();
        canvasGeo.updated = true;
    });
    mainWindow();
    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);
    viewports();
    gameView->gui();
    assets->gui();
    registry->gui();
    modifiers->gui();

    game.load();
    if (game.updateGame)
    {
        game.updateGame();
    }
}

void Editor::mainWindow()
{
    const float TOOLBAR_HEIGHT = 50;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    auto pos = viewport->GetWorkPos();
    auto size = viewport->GetWorkSize();

    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    const int STATUS_BAR_HEIGHT = 20;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3.0f, 3.0f));

    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y));
    ImGui::SetNextWindowSize(ImVec2(size.x, TOOLBAR_HEIGHT));
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::Begin("main", nullptr,
                 ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);
    menu();

    toolbar();


    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + TOOLBAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y - STATUS_BAR_HEIGHT - TOOLBAR_HEIGHT + 20));
    ImGui::SetNextWindowViewport(viewport->ID);
    window_flags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    //    ImGui::PopStyleVar(3);
    ImGuiIO &io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    ImGui::End();

    ImGui::SetNextWindowPos(ImVec2(pos.x, pos.y + size.y - STATUS_BAR_HEIGHT));
    ImGui::SetNextWindowSize(ImVec2(size.x, STATUS_BAR_HEIGHT - 3.0));
    ImGui::SetNextWindowViewport(viewport->ID);
    //     ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    //     ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    //     ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(3.0f, 3.0f));

    ImGui::Begin("StatusBar", 0,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);
    ImGui::Text("Fps: %.1f", ImGui::GetIO().Framerate);
    ImGui::End();
    ImGui::PopStyleVar(3);
}

template <typename C>
bool hasParentComponent(Registry* r, Handle entity, bool includeSelf = false)
{
    Handle parent = entity;
    if (!includeSelf)
    {
        auto rel = r->getPtr<Relation>(entity);
        parent = rel ? rel->parent : 0;
    }
    while (parent)
    {
        if (r->getPtr<C>(parent))
        {
            return true;
        }
        auto rp = r->getPtr<Relation>(parent);
        parent = rp ? rp->parent : 0;
    }
    return false;
}

void Editor::menu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", ""))
            {
                auto fn = EditorUtils::openFileName(".json");
                if (!fn.empty())
                {
                    std::ifstream is(fn);
                    json j;
                    is >> j;
                    r->fromJson(j);                    
                }
            }
            if (ImGui::MenuItem("Save", ""))
            {
                auto fn = EditorUtils::saveFileName(".json");
                if (!fn.empty())
                {                    
                    json j;   
                    std::vector<int> entities;
                    for (auto kv : r->entities())
                    {
                        auto inf = r->getPtr<Info>(kv.first);
                        if (inf && inf->scope != 0)
                            continue;
                        if (hasParentComponent<Proto>(r, kv.first, true))
                            continue;
                        if (hasParentComponent<Ref>(r, kv.first))
                            continue;
                        entities.push_back(kv.first);
                    }
                    
                    r->toJson(entities, j);
                    LOG_F(INFO, "serialized: %s", j.dump(4).c_str());
                    std::ofstream os(fn);
                    os << j.dump(4);
                }                
            }
            ImGui::MenuItem("Save As", "");
            ImGui::Separator();
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void Editor::toolbar()
{
    modifiers->toolbar();
    ImGui::SameLine();
    ImGui::Dummy(ImVec2(30, 0));
    ImGui::SameLine();
    gameView->toolbar();
}

void Editor::viewports()
{
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigWindowsMoveFromTitleBarOnly = true;
    static auto frame = 0;
    frame++;
    static vec3 m1, m2;

    auto &edState = r->get<EditorState>(editor);

    static mat4 transform = glm::identity<mat4>();
    // glm::rotate(PI2 / 2, vec3(0, 1, 0));
    static Handle focused;
    static vec3 scaling(1.0);
    static quat rotation = glm::identity<quat>();
    static vec3 translation(0.0f);

    r->each<Viewport, RenderPass, Camera>(
        [&](Handle entity, Viewport &vp, RenderPass &pass, Camera &camera) {
            if (focused == entity)
            {
                //                translationGizmo(transform, translation);
                //                transform = glm::translate(translation) * transform;
                //                translation = vec3(0);

                // scaleGizmo(transform, scaling);
                // rotateGizmo(transform, rotation);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            bool isOpen = true;
            if (ImGui::Begin((vp.name).c_str(), &isOpen))
            {

                ImVec2 vMin = ImGui::GetWindowContentRegionMin();
                ImVec2 vMax = ImGui::GetWindowContentRegionMax();

                vMin.x += ImGui::GetWindowPos().x;
                vMin.y += ImGui::GetWindowPos().y;
                vMax.x += ImGui::GetWindowPos().x;
                vMax.y += ImGui::GetWindowPos().y;
                auto region = ImGui::GetWindowContentRegionMax();
                ImVec2 size = ImVec2(region.x, glm::max<float>(1, vMax.y - vMin.y));

                if (ImGui::IsWindowFocused())
                {
                    if (focused != entity)
                    {
                        focused = entity;
                        LOG_F(INFO, "Focused %d", entity);
                    }
                    auto mouse = ImGui::GetMousePos();
                    mouse.x -= vMin.x;
                    mouse.y -= vMin.y;

                    // LOG_F(INFO, "mouse %f, %f", mouse.x / size.x, 1.0 - mouse.y / size.y);

                    auto ndcMouse = vec2(mouse.x / size.x, 1.0 - mouse.y / size.y) * 2.0f - vec2(1.0f);
                    auto invCam = glm::inverse(camera.projection * camera.view);
                    auto a = invCam * vec4(ndcMouse.x, ndcMouse.y, -1.0f, 1.0f);
                    auto b = invCam * vec4(ndcMouse.x, ndcMouse.y, 1.0f, 1.0f);
                    a = a / a.w;
                    b = b / b.w;
                    Ray mouseRay = Ray(normalize(b - a), a);
                    if (io.MouseDown[0])
                    {
                        m1 = vec3(a);
                        m2 = vec3(a + normalize(b - a) * 100.0f);
                        //                        edState.mouse = vec2(mouse.x, mouse.y);
                        //                        edState.mouseRay = mouseRay;
                    }

                    edState.mouse = vec2(mouse.x / size.x, 1.0 - mouse.y / size.y);
                    edState.mouseRay = mouseRay;
                    edState.mouseTracked = false;
                    for (size_t i = 0; i < 3; i++)
                    {
                        edState.mouseButtons[i] = io.MouseDown[i];
                    }
                }

                ImGui::Image((void *)(intptr_t)RenderSys::uiRenderTargetHandle(entity), ImVec2(size.x, size.y),
                             ImVec2(0, 1), ImVec2(1, 0));
                // ImGui::BeginChild("Test");
                ImDrawList *drawList = ImGui::GetWindowDrawList();
                auto pos = ImGui::GetWindowPos();
                if (ImGui::IsItemHovered())
                {
                    if (io.MouseDown[1])
                    {
                        camera.rotation.y += io.MouseDelta.x * 0.005f;
                        camera.rotation.x += io.MouseDelta.y * 0.005f;
                    }
                    camera.distance += io.MouseWheel;
                }
                if (!io.MouseDown[0] && frame > 1)
                {
                    pass.size = ivec2(size.x, size.y);
                }
                camera.aspect = float(size.x) / float(size.y);
                camera.update();
                // LOG_F(INFO, "Size: %s", glm::to_string(size).c_str());
            }
            ImGui::End();
            ImGui::PopStyleVar();
            if (!isOpen)
            {
                r->release(entity);
            }
        });
    
}

Editor *Editor::instance()
{
    return _instance;
}

void Editor::createViewport(std::string name)
{
    static int vpId = 8;
    vpId *= 2;

    Camera camera;
    camera.distance = 10;
    camera.lookAt = vec3(0.0f);
    camera.rotation = {1.00439012, 0.000000000, 4.18739033};
    camera.aspect = 1.0f;

    //    camera.view = glm::lookAt(vec3(10, 50, -50), vec3(0, 0, 0), vec3(0, 1, 0));
    camera.projection = glm::perspective(45.0f, 1.0f, 0.1f, 250.0f);
    camera.update();

    RenderPass pass;
    pass.size = {0, 0};
    pass.attachments.push_back(Attachment{RenderDescriptorType::COLOR, AttachmentType::Color});
    pass.attachments.push_back(Attachment{RenderDescriptorType::SSAO, AttachmentType::Color});
    pass.attachments.push_back(Attachment{RenderDescriptorType::DEPTH, AttachmentType::Depth});
    pass.attachments.push_back(Attachment{RenderDescriptorType::NORMALS, AttachmentType::Color});
    pass.attachments.push_back(Attachment{RenderDescriptorType::POSITION, AttachmentType::Color});

    pass.subpasses.resize(4);

    pass.subpasses[0].pipeline = Pipeline{"shaders/gbuf"};
    pass.subpasses[0].outputs.push_back(RenderDescriptorType::POSITION);
    pass.subpasses[0].outputs.push_back(RenderDescriptorType::NORMALS);
    pass.subpasses[0].outputs.push_back(RenderDescriptorType::DEPTH);

    pass.subpasses[1].pipeline = Pipeline{"shaders/ssao2"};
    pass.subpasses[1].pipeline.screenSpace = true;
    pass.subpasses[1].inputs.push_back(RenderDescriptorType::NORMALS);
    pass.subpasses[1].inputs.push_back(RenderDescriptorType::POSITION);
    pass.subpasses[1].outputs.push_back(RenderDescriptorType::SSAO);

    pass.subpasses[2].pipeline = Pipeline{"shaders/textured.material"};
    pass.subpasses[2].inputs.push_back(RenderDescriptorType::NORMALS);
    pass.subpasses[2].outputs.push_back(RenderDescriptorType::COLOR);
    pass.subpasses[2].outputs.push_back(RenderDescriptorType::DEPTH);

    pass.subpasses[3].pipeline = Pipeline{"shaders/basic.color"}; // Gizmos
    pass.subpasses[3].pipeline.layers = vpId;
    pass.subpasses[3].outputs.push_back(RenderDescriptorType::COLOR);
    pass.subpasses[3].clearMask = 0;


    Geometry canvasGeo; 
    canvasGeo.bufferType = Geometry::BufferType::Dynamic;
    canvasGeo.type = Geometry::GeometryType::Triangles;
    canvasGeo.layer = vpId;

    //auto canvas = r->createEntity(Transform{{0, -.05, 0}, mat4(), vec3(1.0)}, canvasGeo);

    Viewport vp;
    vp.name = name;
    r->createEntity(
        Info{name, 1}, vp, pass, camera, Transform{{0, -.05, 0}, mat4(), vec3(1.0)}, canvasGeo);
}

 Game::Game()
{
    libraryPath = L"game.dll";
    modified = 0;
 }


void Game::load()
{    
    #ifdef WIN32
    static int i = 0;
    HANDLE hDLLFile =
        CreateFileW(L"game.dll", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL, NULL);

    if (hDLLFile == INVALID_HANDLE_VALUE)
    {
        return;
    }

    uint64_t fileTime;
    GetFileTime(hDLLFile, NULL, NULL, (FILETIME *)&fileTime);
    CloseHandle(hDLLFile);    

    i++;
    if (fileTime > modified || i==20)
    {
        std::wstring tmpname = libraryPath + L".tmp.dll";
        if (modified)
        {
            stopGame();
            FreeLibrary(HMODULE(module));
        }
        
        if (CopyFileW(libraryPath.c_str(), tmpname.c_str(), FALSE))
        {
            HMODULE handle = LoadLibraryW(tmpname.c_str());
            module = (uint64_t)handle;
            initGame = (Game::InitGame)GetProcAddress(handle, "initGame");
            destroyGame = (Game::DestroyGame)GetProcAddress(handle, "destroyGame");
            pauseGame = (Game::PauseGame)GetProcAddress(handle, "pauseGame");
            startGame = (Game::StartGame)GetProcAddress(handle, "startGame");
            stopGame = (Game::StopGame)GetProcAddress(handle, "stopGame");
            updateGame = (Game::UpdateGame)GetProcAddress(handle, "updateGame");
            resizeGame = (Game::ResizeGame)GetProcAddress(handle, "resizeGame");
            textureHandle = (Game::TextureHandle)GetProcAddress(handle, "textureHandle");
            initGame(Engine::instance());            
            modified = fileTime;
        }
    }
    #endif
}
