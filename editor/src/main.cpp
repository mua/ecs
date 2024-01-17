#include "Application.h"
#include "Engine.h"
#include "Systems/UISys.h"

#include "components/Render.h"
#include "imgui.h"
#include "systems/RenderSys.h"
#include "systems/ResourceSys.h"
#include <iostream>

#include "ImGuizmo.h"
#include "components/core.h"
#include "components/transform.h"
#include "systems/loadsys.h"
#include "utils/filenode.h"
#include "views/assets.h"
#include "views/registry.h"
#include <loguru.hpp>
#include <string>

#include "editor.h"
#include "systems/transformsys.h"


typedef void(*InitGameProc)(void);

int main(int argc, char *argv[])
{
    loguru::init(argc, argv);

    LOG_F(INFO, "Starting");

    Application app;
    auto engine = Engine::instance();

    engine->addSystem(new Editor());
    engine->addSystem(new TransformSys());
    engine->addSystem(new RenderSys());
    engine->addSystem(new ResourceSys());
    engine->addSystem(new LoadSys());
    

    //engine->registry.createEntity(Ref{"Debug/resources/gltf/BoxTextured/glTF/BoxTextured.gltf"});
    //engine->registry.createEntity(Ref{"resources/test.glb"});

    auto geo = Geometry();
    geo.makeGrid({100, 100}, {100, 100});
    engine->registry.createEntity(Transform{{0, -.05, 0}, mat4(), vec3(1.0)}, geo, Info{"grid", 1});



    //     engine->registry.entity<Position>(1, [&](auto pos) {
    //         // std::cout << "entity";
    //     });
    //     engine->registry.entity<Position, Mesh, PhysicsBody>(1, [&](auto pos, auto mesh, auto body) {
    //         // std::cout << "entity";
    //     });

    //     auto &[pos, mesh] = engine->registry.getEntity<Position, Mesh>(1);
    //     pos.x = 3;
    //
    //     {
    //         auto &[pos, mesh] = engine->registry.getEntity<Position, Mesh>(1);
    //         // std::cout << pos.x;
    //     }

    app.run();
}
