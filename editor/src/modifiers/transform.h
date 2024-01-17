#ifndef transform_modifier_h__
#define transform_modifier_h__

#include "modifier.h"
#include "components/transform.h"

struct TransformModifier : Modifier
{
    bool isAvailable() override;
};

struct TranslationModifier: TransformModifier
{


bool process(Handle viewport) override;
};

struct RotationModifier : TransformModifier
{


bool process(Handle viewport) override;
};

struct ScalingModifier : TransformModifier
{


bool process(Handle viewport) override;
};

#endif // transform_modifier_h__
