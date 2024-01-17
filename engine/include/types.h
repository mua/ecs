#ifndef types_h__
#define types_h__

#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <vector>

#include "serialize.h"

typedef glm::ivec2 ivec2;
typedef glm::ivec3 ivec3;
typedef glm::ivec4 ivec4;
typedef glm::vec2 vec2;
typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef glm::mat4 mat4;
typedef glm::mat3 mat3;
typedef glm::quat quat;

const float PI2 = glm::pi<float>() * 0.5f;
const float PI = glm::pi<float>();

typedef int Handle;

struct Vertex
{
    vec3 pos;
    vec3 normal;
    vec4 color = vec4(1.0);
    vec2 uv;
};

struct Color : public vec4
{
    using vec4::vec4;
    static Color red;
    static Color green;
    static Color blue;
    static Color yellow;
    static Color orange;
    static Color white;
    Color(const vec4& color)
    {
        r = color.r;
        g = color.g;
        b = color.b;
        a = color.a;
    }
};

struct Ray
{
    vec3 n;
    vec3 p;
    Ray(): n(0.0f), p(0.0f) {};
    Ray(vec3 n, vec3 p) : n(n), p(p)
    {
    }
    vec3 cast(Ray plane);
    float rayDistance(Ray b);
    float pointDistance(vec3 b);
    vec3 rayProj(Ray b);
    vec3 rayProj(Ray b, vec3 normal);
    float rayClosestPointToLineSegment(vec3 a, vec3 b, float r);
    float rayClosestPointToLineSegment(vec3 a, vec3 b);
};

Ray operator*(const mat4 &mat, const Ray &ray);

const float NO_INTERSECT = HUGE_VALF;

struct BBox
{
    vec3 min;
    vec3 max;
    bool empty;

    BBox() : empty(true)
    {
    }
    BBox(vec3 min, vec3 max) : min(min), max(max), empty(false)
    {
    }
    
    float intersect(const Ray &r) const;
    void addPoint(vec3 point);
    std::vector<vec3> vertices() const;

    static Encoder serializer()
    {
        return serialize(
            "min", &BBox::min, 
            "max", &BBox::max, 
            "empty", &BBox::empty)
            ;
    }
};

BBox operator*(const mat4 &mat, const BBox &box);
BBox operator+(const BBox &a, const BBox &b);
BBox operator+=(BBox &a, const BBox &b);


#endif // types_h__