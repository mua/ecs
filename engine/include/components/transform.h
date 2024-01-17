#ifndef transform_h__
#define transform_h__

#include "types.h"
#include "storage.h"

struct Transform
{
    vec3 position = vec3(0.0f);
    quat rotation = glm::identity<quat>();
    vec3 scaling = vec3(1.0f);

    mat4 parentWorldTransform = glm::identity<mat4>();

    Transform()
    {
    }

    Transform(vec3 position, quat rotation, vec3 scaling)
        : position(position), rotation(rotation), scaling(scaling)
    {    
    }

    mat4 matrix() const
    {
        return glm::translate(position) * glm::mat4_cast(rotation) * glm::scale(scaling);
    }
    
    void matrix(mat4 matrix)
    {
        vec3 skew;
        vec4 perspective;
        glm::decompose(matrix, scaling, rotation, position, skew, perspective); 
    }

    mat4 worldMatrix()
    {
        return parentWorldTransform *matrix();
    }
};
REGISTER_COMPONENT(Transform)

inline Transform operator+(const Transform &a, const Transform &b)
{
    return Transform(a.position + b.position, a.rotation + b.rotation, a.scaling + b.scaling);
}

inline Transform operator*(const Transform &a, const Transform &b)
{
    auto pos = vec3(a.matrix() * vec4(b.position, 1.0f));
    auto rot = a.rotation * b.rotation;
    auto scale = a.scaling * b.scaling;
    return Transform(pos, rot, scale);
}

#endif // transform_h__
