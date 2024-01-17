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

#include <loguru.hpp>
#include <string>

#include "systems/transformsys.h"

int main(int argc, char *argv[])
{
    loguru::init(argc, argv);

    LOG_F(INFO, "Starting");

    Application app;
    auto engine = Engine::instance();

    engine->addSystem(new TransformSys());
    //engine->addSystem(new RenderSys());
    //engine->addSystem(new ResourceSys());
    //engine->addSystem(new LoadSys());

    auto geo = Geometry();
    geo.makeGrid({100, 100}, {100, 100});
    engine->registry.createEntity(Transform{{0, -.05, 0}, mat4(), vec3(1.0)}, geo, Info{"grid", 1});
    app.run();
}
