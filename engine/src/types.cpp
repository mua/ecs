#include "types.h"
#include <vector>
#include "storage.h"

Color Color::red = Color(1, 0, 0, 1);
Color Color::green = Color(0, 1, 0, 1);
Color Color::blue = Color(0, 0, 1, 1);
Color Color::yellow = Color(1, 1, 0, 1);
Color Color::orange = Color(1, 0.55, 0.04, 1);
Color Color::white = Color(1, 1, 1, 1);

REGISTER_COMPONENT(BBox) 

Ray operator*(const mat4 &mat, const Ray &ray)
{
    return Ray(mat3(mat) * ray.n, mat * vec4(ray.p, 1.0f));
}

vec3 Ray::cast(Ray plane)
{
    auto d = glm::dot(plane.p - p, plane.n);
    return (d / glm::dot(plane.n, n)) * n + p;
}

float Ray::rayDistance(Ray b)
{
    auto c = glm::cross(n, b.n);
    return glm::dot(p - b.p, c) / glm::length(c);
}

float Ray::pointDistance(vec3 b)
{
    auto d = b - p;
    auto t = glm::dot(n, d) * n + p;
    return glm::length(b - t);
}

vec3 Ray::rayProj(Ray b)
{
    auto d = p - b.p;
    auto cp = glm::normalize(glm::cross(glm::cross(d, b.n), n));
    // cp = vec3(0, 0, 1);
    auto t = b.cast(Ray(cp, p)) - p;
    return glm::dot(t, n) * n + p;
}

vec3 Ray::rayProj(Ray b, vec3 normal)
{
    auto t = b.cast(Ray(normal, p)) - p;
    return glm::dot(t, n) * n + p;
}

float Ray::rayClosestPointToLineSegment(vec3 a, vec3 b, float r)
{
    auto axRay = Ray(glm::normalize(b-a), a);
    auto normal = glm::normalize(glm::cross(axRay.n, n));
    auto plane = glm::cross(normal, axRay.n);
    auto t = glm::dot(axRay.p - p, plane) / glm::dot(plane, n);
    auto cp = t * n + p;
    auto distance = glm::dot(cp - axRay.p, normal);
    auto axp = glm::dot(cp - axRay.p, axRay.n);
    return glm::abs(distance)<=r ? axp : -1;
}

float Ray::rayClosestPointToLineSegment(vec3 a, vec3 b)
{
    auto axRay = Ray(glm::normalize(b - a), a);
    auto normal = glm::normalize(glm::cross(axRay.n, n));
    auto plane = glm::cross(normal, axRay.n);
    auto t = glm::dot(axRay.p - p, plane) / glm::dot(plane, n);
    auto cp = t * n + p;
    auto distance = glm::dot(cp - axRay.p, normal);
    return glm::dot(cp - axRay.p, axRay.n);
}

//////////////////////////////////////////////////////////////////////////


float BBox::intersect(const Ray &r) const
{
    using namespace std;

    float tmin = (min.x - r.p.x) / r.n.x;
    float tmax = (max.x - r.p.x) / r.n.x;

    if (tmin > tmax)
        swap(tmin, tmax);

    float tymin = (min.y - r.p.y) / r.n.y;
    float tymax = (max.y - r.p.y) / r.n.y;

    if (tymin > tymax)
        swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax))
        return NO_INTERSECT;

    if (tymin > tmin)
        tmin = tymin;

    if (tymax < tmax)
        tmax = tymax;

    float tzmin = (min.z - r.p.z) / r.n.z;
    float tzmax = (max.z - r.p.z) / r.n.z;

    if (tzmin > tzmax)
        swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax))
        return NO_INTERSECT;

    if (tzmin > tmin)
        tmin = tzmin;

    if (tzmax < tmax)
        tmax = tzmax;

    return tmin;
}

void BBox::addPoint(vec3 point)
{
    if (empty)
    {
        empty = false;
        min = max = point;
    }
    else
    {
        min = glm::min(point, min);
        max = glm::max(point, max);
    }
}

std::vector<vec3> BBox::vertices() const
{
    std::vector<vec3> ret;
    for (int i = 0; i < 8; i++)
    {
        ret.push_back(vec3((i & 1 ? min : max).x, (i & 2 ? min : max).y, (i & 4 ? min : max).z));
    }
    return ret;
}

BBox operator*(const mat4 &mat, const BBox &box)
{
    BBox ret;
    auto verts = box.vertices();
    for (int i = 0; i < 8; i++)
    {
        ret.addPoint(mat * vec4(verts[i], 1.0f));
    }
    return ret;
}

BBox operator+(const BBox &a, const BBox &b)
{
    return BBox(glm::min(a.min, b.min), glm::max(a.max, b.max));
}

BBox operator+=(BBox &a, const BBox &b)
{
    if (a.empty)
    {
        a = b;
        return b;
    }
    if (b.empty)
    {
        return a;
    }
    a.min = glm::min(a.min, b.min);
    a.max = glm::max(a.max, b.max);
    return a + b;
}
