#include "transform.h"

#include "engine.h"
#include "types.h"
#include "editor.h"
#include "components/render.h"
#include "loguru.hpp"

enum class GizmoTipType
{
    Cone,
    Box
};

bool drawGizmo(mat4 transform, Ray &movementPlane, vec3 &axis, bool readOnly, GizmoTipType tipType, Handle viewport)
{    
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto &[geo, camera] = Engine::instance()->registry.getEntity<Geometry, Camera>(viewport);

    
    vec3 center = transform * vec4(0.0, 0.0, 0.0, 1.0);
    bool active = false;
    static bool highlighed[6] = {0,0,0,0,0,0};
    int indicator = 0;
    if (!readOnly)
    {
        memset(highlighed, 0, sizeof(highlighed));
    }
    
    mat3 scale = glm::scale(vec3(glm::length(vec3(glm::inverse(camera.view) * vec4(0.0, 0.0, 0.0, 1.0)) - center) / 10.0f));

    for (auto ax : std::vector<std::pair<vec3, Color>>{
             {vec3(1, 0, 0), Color::red}, {vec3(0, 1, 0), Color::green}, {vec3(0, 0, 1), Color::blue}})
    {
        vec3 a = center;
        vec3 b = transform * vec4(scale * ax.first, 1.0);
        auto t = edState.mouseRay.rayClosestPointToLineSegment(a, b, 0.125);
        auto high = t > 0;
        auto col = (high || highlighed[indicator]) ? Color::yellow : ax.second;
        geo.addLineSegment(a, b, col, 0.05f);
        highlighed[indicator++] |= high;

        mat3 rot;
        rot[2] = normalize(b - a);
        rot[1] = glm::cross(
            rot[2], glm::normalize(glm::length(rot[2] - vec3(0, 1, 0)) > 0.001 ? vec3(0, 1, 0) : vec3(1, 0, 0)));
        rot[0] = glm::cross(rot[1], rot[2]);
        rot = rot * scale;

        switch (tipType)
        {
        case GizmoTipType::Cone:
            rot[2] *= 2.5f;
            geo.addCone(glm::translate(b) * mat4(rot * 0.125f), col);
            break;
        case GizmoTipType::Box:
            geo.addBox(glm::translate(b) * mat4(rot * 0.25f), col);
            break;
        default:
            break;
        }

        if (high && edState.mouseButtons[0] && !readOnly)
        {
            axis = normalize(b - a);
            movementPlane = Ray(glm::cross(glm::cross(axis, edState.mouseRay.n), axis), a);
            active = true;
        }
    }
    for (auto &axes : {
             std::pair{vec3(1, 0, 0), vec3(0, 1, 0)},
             std::pair{vec3(1, 0, 0), vec3(0, 0, 1)},
             std::pair{vec3(0, 1, 0), vec3(0, 0, 1)},
         })
    {
        glm::mat3 rot;
        rot[0] = axes.first;
        rot[1] = axes.second;
        rot[2] = glm::cross(axes.first, axes.second);
        rot = rot * scale;

        auto bbox = BBox(rot * vec3(0.20, 0.20, 0), rot * vec3(0.5, 0.5, 0.01));
        auto high = bbox.intersect(glm::inverse(transform) * edState.mouseRay) != NO_INTERSECT;        
        geo.addBox(bbox.min, bbox.max, transform, Color::yellow * ((high || highlighed[indicator]) ? 1.0f : 0.5f));
        highlighed[indicator++] |= high;
        if (high && edState.mouseButtons[0] && !readOnly)
        {
            movementPlane = Ray(glm::cross(axes.first, axes.second), center);
            axis = vec3(0.0);
            active = true;
        }
    }
    return active;
}

bool translationGizmo(mat4 &space, vec3 &translation, Handle viewport)
{
    static vec3 axis;
    static Ray movementPlane;
    static vec3 oldPos;
    static bool started = false;
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    
    bool active = drawGizmo(space * glm::translate(translation), movementPlane, axis, started, GizmoTipType::Cone, viewport);

    if (!started && active)
    {
        oldPos = edState.mouseRay.cast(movementPlane);
        started = true;
    }
    started = started && edState.mouseButtons[0];

    if (started)
    {
        auto pos = edState.mouseRay.cast(movementPlane);
        auto movement = pos - oldPos;
        if (glm::length(axis) > 0.01)
            movement = glm::dot(axis, movement) * axis;
        translation += movement;
        oldPos = pos;
    }

    return started;
}

bool scaleGizmo(mat4 space, vec3 &scaling, Handle viewport)
{
    static vec3 axis;
    static Ray movementPlane;
    static vec3 oldPos;
    static bool started = false;
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    
    bool active = drawGizmo(space * glm::scale(scaling), movementPlane, axis, started, GizmoTipType::Box, viewport);

    if (!started && active)
    {
        oldPos = edState.mouseRay.cast(movementPlane);
        started = true;
    }
    started = started && edState.mouseButtons[0];

    if (started)
    {
        auto pos = edState.mouseRay.cast(movementPlane);
        auto movement = pos - oldPos;
        if (glm::length(axis) > 0.01)
            movement = glm::dot(axis, movement) * axis;
        oldPos = pos;
        scaling += glm::inverse(mat3(space)) * movement;
    }

    return started;
}

bool drawRotationGizmo(mat4 space, quat &rotation, Ray &movementPlane, mat3 &axisTransform, bool started,
                       Handle viewport)
{
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto &[geo, camera] = Engine::instance()->registry.getEntity<Geometry, Camera>(viewport);
    vec3 center = space * vec4(0.0, 0.0, 0.0, 1.0);
    mat3 t = mat3(space) * mat3(rotation);
    const float w = 0.025f;
    struct Ax
    {
        mat3 transform;
        Color color;
    };
    bool active = started;

    mat3 scale =
        glm::scale(vec3(glm::length(vec3(glm::inverse(camera.view) * vec4(0.0, 0.0, 0.0, 1.0)) - center) / 5.0f));

    for (auto ax : {Ax{mat3{t[1], t[2], t[0]}, Color::red}, Ax{mat3{t[2], t[0], t[1]}, Color::green},
                    Ax{mat3{t[0], t[1], t[2]}, Color::blue}})
    {
        auto plane = Ray(glm::normalize(ax.transform * vec3(0, 0, 1)), center);
        auto hit = edState.mouseRay.cast(plane);
        //LOG_F(INFO, "Hit: %s", glm::to_string(hit).c_str());
        auto r = glm::length(glm::inverse(scale) * hit - glm::inverse(scale) * center);
        bool high;
        if (started)
        {
            high = glm::length(plane.n - movementPlane.n) < 0.001;
        }
        else
        {
            high = r < 1.1f && r > 0.9;
            if (high)
            {
                movementPlane = plane;
                axisTransform = ax.transform;
                active = true;
            }
        }
        geo.addCircle(center, 1, w, ax.transform * scale, high ? Color::yellow : ax.color);
    }
    return active;
}

bool rotateGizmo(mat4 space, quat &rotation, Handle viewport)
{
    static Ray movementPlane;
    static vec3 oldPos;
    static bool started = false;
    static float startAngle;
    static float angleDelta;
    static mat3 axisTransform = glm::identity<mat3>();

    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto &[geo, camera] = Engine::instance()->registry.getEntity<Geometry, Camera>(viewport);

    bool active = drawRotationGizmo(space, rotation, movementPlane, axisTransform, started, viewport);
    auto center = vec3(space * vec4(0, 0, 0, 1));

    if (!started && active)
    {
        oldPos = edState.mouseRay.cast(movementPlane) - center;
        auto copos = glm::inverse(axisTransform) * oldPos;
        startAngle = glm::atan(copos.y, copos.x);
        angleDelta = 0;
        started = true;
    }
    started = started && edState.mouseButtons[0];

    if (started)
    {
        auto pos = edState.mouseRay.cast(movementPlane) - center;
        auto cpos = glm::inverse(axisTransform) * pos;
        auto copos = glm::inverse(axisTransform) * oldPos;
        // auto delta = glm::orientedAngle(normalize(oldPos), normalize(pos), movementPlane.n);
        float oldAngle = glm::atan(copos.y, copos.x);
        float newAngle = glm::atan(cpos.y, cpos.x);
        rotation = glm::rotate(glm::identity<quat>(), newAngle - oldAngle, movementPlane.n) * rotation;
        angleDelta += newAngle - oldAngle;
        oldPos = pos;
        geo.addCircle(center, 0.99, 0.025f, axisTransform, Color::yellow * 0.75f, true, startAngle,
                      startAngle + angleDelta);
    }

    return started;
}

//////////////////////////////////////////////////////////////////////////////////

bool TransformModifier::isAvailable()
{
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto transforms = Engine::instance()->registry.get<Transform>(edState.selection);
    return transforms.size() > 0;
}

bool TranslationModifier::process(Handle viewport)
{
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto transforms = Engine::instance()->registry.get<Transform>(edState.selection);
    auto space = glm::identity<mat4>();
    bool started = translationGizmo(space, transforms[edState.selection[0]]->position, viewport);
    edState.mouseTracked |= started;
    return started;
}

bool RotationModifier::process(Handle viewport)
{
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto transforms = Engine::instance()->registry.get<Transform>(edState.selection);
    auto t = transforms[edState.selection[0]];
    bool started = rotateGizmo(glm::translate(t->position), transforms[edState.selection[0]]->rotation, viewport);
    edState.mouseTracked |= started;
    return started;
}

bool ScalingModifier::process(Handle viewport)
{
    auto &edState = Engine::instance()->registry.get<EditorState>(Editor::instance()->editor);
    auto transforms = Engine::instance()->registry.get<Transform>(edState.selection);
    auto t = transforms[edState.selection[0]];
    bool started = scaleGizmo(t->matrix(), t->scaling, viewport);
    edState.mouseTracked |= started;
    return started;
}
