#include "selection.h"

#include "engine.h"
#include "components/transform.h"

#include "imgui.h"
#include "components/render.h"
#include "editor.h"
#include "components/core.h"
#include "loguru.hpp"

Ray mouseRay;

bool SelectionModifier::process(Handle viewport)
{
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto &geo = Engine::instance()->registry.get<Geometry>(viewport);
    auto io = ImGui::GetIO();
    static bool boxStarted = false;

    for (auto selected : edState.selection)
    {
        auto &[transform, box] = Engine::instance()->registry.getEntity<Transform, BBox>(selected);
        if (&box)
            geo.addBBox(box, transform.matrix());
    }

    if (!(edState.mouse.x < 1 && edState.mouse.x > 0 && edState.mouse.y < 1 && edState.mouse.y > 0))
    {
        return false;
    }
    if (!edState.mouseTracked && io.MouseClicked[0])
    {
        mouseRay = edState.mouseRay;
        LOG_F(INFO, "mouse: %s", glm::to_string(edState.mouse).c_str());

        edState.selection.clear();
        Handle clicked = 0;
        float hitDistance = NO_INTERSECT;
        Engine::instance()->registry.each<Transform, BBox>([&](Handle entity, Transform &t, BBox &box) {
            auto inf = Engine::instance()->registry.getPtr<Info>(entity);
            if (inf && !inf->active)
                return;
            auto hit = (t.matrix() * box).intersect(edState.mouseRay);
            if (hit < hitDistance)
            {
                clicked = entity;
                hitDistance = hit;
            }
        });
        if (clicked)
        {
            edState.selection.clear();
            edState.selection.push_back(clicked);
        }
    }
    else
    {
        if (!boxStarted && io.MouseDown[0])
        {
            LOG_F(INFO, "select box started: %s", glm::to_string(edState.mouse).c_str());
            boxStarted = true;
        }
        if (boxStarted && !io.MouseDown[0])
        {
            LOG_F(INFO, "select box ended: %s", glm::to_string(edState.mouse).c_str());
            boxStarted = false;
        }
        if (boxStarted)
        {
            //geo.addBBox(box, transform.matrix());
        }
    }
    //geo.addLineSegment(mouseRay.p, mouseRay.p + mouseRay.n * 100.0f);
    return false;
}

bool SelectionModifier::isAvailable()
{
    return true;
}
