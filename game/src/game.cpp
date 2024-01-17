#include "game.h"

#include "engine.h"
#include "loguru.hpp"
#include "systems/system.h"
#include <chrono>
#include "components/render.h"
#include "components/core.h"
#include "components/transform.h"
#include "systems/transformsys.h"
#include "systems/rendersys.h"
#include "systems/resourcesys.h"
#include "systems/loadsys.h"
#include <iostream>

bool started = false;
bool paused = false;
Handle viewport = 0;

class GameSys : public System
{
  public:
    void start() override
    {
        LOG_F(INFO, "game system started!");

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

        pass.subpasses.resize(3);

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

        viewport = r->createEntity(Info{"Game", 1}, pass, camera);
    }

    void process() override
    {
        auto time =
            std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count();
        if (!(time % 1000))
        {
            LOG_F(INFO, "game system running! 11");
        }
        r->each<Transform>([time](Handle entity, Transform &t) { 
            t.position += ((time % 1000 - 500) / 5000.0f);
        });
    }
};

GameSys *gameSys = nullptr;
Engine *engine = nullptr;
Engine *sEngine = nullptr;

GAME_EXPORT void initGame(Engine *engine_) noexcept
{
    LOG_F(INFO, "game initialized 25!");    
    glewInit();

    sEngine = engine_;
    engine = new Engine();
    gameSys = new GameSys();
    engine->addSystem(gameSys);
    started = false;
    paused = false;

    engine->addSystem(new TransformSys());
    engine->addSystem(new RenderSys());
    engine->addSystem(new ResourceSys());

    for (auto sys: engine->systems)
    {
//        sys->r = engine->registry;
//        assert(&sys->r == &engine->registry);
    }
    //engine->addSystem(new LoadSys());    

    
}

GAME_EXPORT void destroyGame() noexcept
{
    delete engine;
    engine = nullptr;
}

GAME_EXPORT void startGame() noexcept
{
    if (!engine)
    {
        initGame(sEngine);
    }
    if (!started)
    {
        engine->registry.copyFrom(sEngine->registry);
//         for (auto sys : engine->systems)
//         {
//             sys->r = engine->registry;
//         }
        for (auto handle : engine->registry.findAll<Pipeline>())
        {
            engine->registry.removeEntity(handle);
        }
        started = true;
        engine->start();
    }
    paused = false;
}

GAME_EXPORT void pauseGame() noexcept
{
    paused = true;
}

GAME_EXPORT void updateGame() noexcept
{
    if (engine && started && !paused)
    {
        engine->loop();
    }
}

GAME_EXPORT void stopGame() noexcept
{
    LOG_F(INFO, "stopped game!");
    for (auto sys : engine->systems)
    {
        delete sys;
    } 
    gameSys = nullptr;
    delete engine;
    engine = nullptr;
    started = false;
}

GAME_EXPORT void *textureHandle() noexcept
{
    RenderPassInstance ins;
    if (engine && engine->registry.get(viewport, ins))
    {
        // auto name = !glTarget.multisampled ? glTarget.texture.textureName : glTarget.blitTexture.textureName;
        return (void *)(intptr_t)ins.attachments.front().texture.textureName;
    }
    return 0;
}

GAME_EXPORT void resizeGame(int width, int height) noexcept
{
    if (started && engine)
    {
        engine->registry.getPtr<RenderPass>(viewport)->size = ivec2(width, height);
        auto cam = engine->registry.getPtr<Camera>(viewport);
        cam->aspect = float(width) / float(height);
        cam->update();
    }
}

int main()
{
    std::cout << "Hello man";
    return 0;
}