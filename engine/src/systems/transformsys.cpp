#include "systems/transformsys.h"

#include "components/core.h"
#include "components/transform.h"
#include "engine.h"


void TransformSys::start()
{
}

void TransformSys::process()
{
    r->each<Relation, Transform>([&](Handle entity, Relation& rel, Transform &transform) {
        if (rel.parent)
        {
            transform.parentWorldTransform = r->get<Transform>(rel.parent).worldMatrix();
        }
    });
}
