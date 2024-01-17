#ifndef render_h__
#define render_h__

#include "types.h"
#include "shapes.h"

#include <gl/glew.h>

#include <string>
#include <vector>
#include "storage.h"

enum RenderDescriptorType
{
    UNKNOWN_TYPE                =-1,
    TRANSFORM_MODEL             = 1,
    TRANSFORM_VIEW              = 2,
    TRANSFORM_PROJECTION        = 3,
    MATERIAL_DIFFUSE            = 4,
    MATERIAL_BASE_COLOR_TEXTURE = 5,

    SSAO_KERNEL                 = 6,
    SSAO_NOISE_TEXTURE          = 7,

    COLOR                       = 8,
    NORMALS                     = 9,
    DEPTH                       = 10,
    POSITION                    = 11,
    SSAO                        = 12,
};

extern const char *RenderDescriptorTypeNames[];

struct Camera
{
    vec3 rotation=vec3(0.0f);
    vec3 center = vec3(0.0f);
    vec3 lookAt = vec3(0.0f);
    float distance = 10.f;
    bool perspective=true;
    double aspect = 1.0;

    mat4 view;
    mat4 projection;

    void update()
    {
        auto center = this->center + lookAt;
        auto r = glm::cos(rotation.x);
        glm::vec3 eye =
            glm::vec3(r * glm::cos(rotation.y + PI2), glm::sin(rotation.x), r * glm::sin(rotation.y + PI2)) * distance;
        view = glm::lookAt(eye + center, center, glm::vec3(0.0f, 1.0f * glm::sign(r), 0.0f));
        /*
        auto cam = glm::rotate(rotation.y, vec3(0, 1, 0)) * glm::rotate(rotation.x, vec3(1, 0, 0)) *
               glm::translate(vec3(0, 0, distance));
        view = glm::inverse(cam);
        */
        if (perspective)
        {
            projection = glm::perspective(glm::radians(60.0), aspect, 0.001, 100.0);
            //projection[1][1] *= -1;
        }
        else
        {
            projection = glm::perspective(glm::radians(45.0), aspect, 0.1, distance * 1.0);
            //projection[1][1] *= -1;
            auto inv = glm::inverse(projection);
            auto tr = inv * vec4(-1.0, -1.0, 1.0, 1.0);
            tr /= tr.w;
            auto bl = inv * vec4(1.0, 1.0, 1.0, 1.0);
            bl /= bl.w;
            projection = glm::ortho(tr.x, bl.x, tr.y, bl.y, 0.01f, 100.0f);
        }
    }
};
REGISTER_COMPONENT(Camera)

struct RenderTarget
{
    ivec2 size;
};
REGISTER_COMPONENT(RenderTarget)

struct Texture
{
    ivec2 size;
    std::string path;
    enum class Type
    {
        Depth,
        RGB,
        RGBA
    } type = Type::RGBA;

    enum class Wrap
    {
        Clamp,
        Repeat
    } wrap = Wrap::Clamp;

    std::vector<unsigned char> pixels;
};
REGISTER_COMPONENT(Texture)

struct Geometry
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Handle material = 0;
    bool updated = false;
    int layer = 1;
    BBox bbox;

    enum class GeometryType
    {
        Lines = 1,
        Triangles = 2
    } type = GeometryType::Triangles;

    enum class BufferType
    {
        Static,
        Dynamic
    } bufferType = BufferType::Static;

    void updateBBox()
    {
        for (auto &v: vertices)
        {
            bbox.addPoint(v.pos);
        }
    }

    void addLine(vec3 a, vec3 b, vec4 color=Color::white)
    {
        Vertex va, vb;
        va.pos = a;
        va.color = color;
        vb.pos = b;
        vb.color = color;
        vertices.push_back(va);
        vertices.push_back(vb);
        indices.push_back(vertices.size() - 2);
        indices.push_back(vertices.size() - 1);
    }

    void addBox(mat4 transform, Color color = Color::white)
    {
        addShape(color, transform, BOX_VERTS, sizeof(BOX_VERTS)/(sizeof(BOX_VERTS[0]) * 3), BOX_INDICES,
                 sizeof(BOX_INDICES) / sizeof(BOX_INDICES[0]));

    }

    void addCone(mat4 transform, Color color = Color::white)
    {
        addShape(color, transform, CONE_VERTS, sizeof(CONE_VERTS) / (sizeof(CONE_VERTS[0]) * 3), CONE_INDICES,
                 sizeof(CONE_INDICES) / sizeof(CONE_INDICES[0]));
    }

    void addBox(vec3 a, vec3 b, mat4 transform = glm::identity<mat4>(), Color color = Color::white)
    {
        addBox(transform * glm::translate((a + b) * 0.5f) * glm::scale(glm::abs(b - a)), color);
    }

    void addShape(Color color, mat4 transform, float vertexList[], int vertexCount, int indexList[], int indexCount)
    {
        Vertex v;
        v.color = color;
        auto indexOffset = vertices.size();
        for (int i = 0; i < vertexCount; i++)
        {
            v.pos = transform * vec4(vertexList[i * 3 + 0], vertexList[i * 3 + 1], vertexList[i * 3 + 2], 1.0);
            vertices.push_back(v);
        }
        for (int i = 0; i < (indexCount ? indexCount : vertexCount); i++)
        {
            indices.push_back(indexOffset + (indexList ? indexList[i] : i));
        }                   
    }

    void addPoly(Color color, mat4 transform, std::vector<vec3> vertices)
    {
        addShape(color, transform, &vertices[0].x, vertices.size(), nullptr, 0);
    }

//     void addBBox(BBox &bbox, mat4 transform = glm::identity<mat4>(), Color color = Color::white)
//     {
//         addBox(bbox.min, bbox.max, transform, color);
//     }    

    void addBBox(BBox &bbox, mat4 transform = glm::identity<mat4>(), Color color = Color::white)
    {
        const float w = 0.025;
        vec3 vs[] = {bbox.min, bbox.max};
        for (auto h : {0, 1})
        {
            addLineStrip(
                {
                    vec3(vs[0].x, vs[h].y, vs[0].z),
                    vec3(vs[0].x, vs[h].y, vs[1].z),
                    vec3(vs[1].x, vs[h].y, vs[1].z),
                    vec3(vs[1].x, vs[h].y, vs[0].z),
                },
                color, w, transform);
        }
        addLineSegment(vec3(bbox.min.x, bbox.max.y, bbox.min.z), vec3(bbox.min.x, bbox.min.y, bbox.min.z), color, w,
                       transform);
        addLineSegment(vec3(bbox.min.x, bbox.max.y, bbox.max.z), vec3(bbox.min.x, bbox.min.y, bbox.max.z), color, w,
                       transform);
        addLineSegment(vec3(bbox.max.x, bbox.max.y, bbox.max.z), vec3(bbox.max.x, bbox.min.y, bbox.max.z), color, w,
                       transform);
        addLineSegment(vec3(bbox.max.x, bbox.max.y, bbox.min.z), vec3(bbox.max.x, bbox.min.y, bbox.min.z), color, w,
                       transform);
    }

    void addLineSegment(vec3 a, vec3 b, Color color = Color::white, float width = 0.1f,
                        mat4 transform = glm::identity<mat4>())
    {
        a = transform * vec4(a, 1);
        b = transform * vec4(b, 1);
        auto d = glm::normalize(b - a);
        mat4 rot = glm::identity<mat4>(); 
        if (!glm::all(glm::epsilonEqual(vec3(0, 0, 1), glm::abs(d), glm::epsilon<float>())))
        {
            auto ax = glm::cross(vec3(0, 0, 1), d);
            auto angle = glm::angle(vec3(0, 0, 1), d);
            rot = glm::rotate(angle, ax);
        }
        mat4 t = glm::translate((a+b)*0.5f) * rot * glm::scale(vec3(width, width, glm::length(a-b)));
        addBox(t, color);
    }

    void addLineStrip(std::vector<vec3> v, vec4 color = Color::white, float width = 0.1f,
                      mat4 transform = glm::identity<mat4>())
    {
        for (size_t i = 0; i < v.size(); i++)
        {
            addLineSegment(v[i], v[(i + 1) % v.size()], color, width, transform);
        }
    }

    void addPath(std::vector<vec3> points, Color color = Color::white)
    {
    }

    void addCircle(vec3 center, float r, float width = 0.1f, mat4 transform = glm::identity<mat4>(), 
        Color color = Color::white, bool filled=false, float startAngle=0, float endAngle = PI*2)
    {
        int segments = 24;
        float da = endAngle - startAngle;
        for (size_t i = 0; i < segments; i++)
        {
            auto a = i*da / segments + startAngle;
            auto b = ((i + 1) * da) / segments + startAngle;
            auto start = vec4(glm::cos(a)*r, glm::sin(a)*r, 0, 1.0);
            auto end = vec4(glm::cos(b)*r, glm::sin(b)*r, 0, 1.0);
            if (width > 0)
            {
                addLineSegment((transform * start) + vec4(center, 0.0f),
                               (transform * end) + vec4(center, 0.0), color,
                               width);
            }
            if (filled)
            {
                addPoly(color, glm::translate(center) * transform, std::vector<vec3>{start, end, vec3(0.0f)});
            }
        }
    }

    void makeGrid(vec2 size, ivec2 cellCount)
    {
        auto cell = size / (vec2)cellCount;
        auto mid = size * 0.5f;
        auto offset = -vec3(mid.x, 0, mid.y);

        for (size_t x = 0; x < cellCount.x; x++)
        {
            vertices.push_back(Vertex{vec3{x * cell.x, 0, 0} + offset});
            vertices.push_back(Vertex{vec3{x * cell.x, 0, size.y} + offset});
        }
        for (size_t y = 0; y < cellCount.y; y++)
        {
            vertices.push_back(Vertex{vec3{0, 0, y * cell.y} + offset});
            vertices.push_back(Vertex{vec3{size.x, 0, y * cell.y} + offset});
        }
        for (auto& vert: vertices)
        {
            vert.color = Color::white;
        }
        type = GeometryType::Lines;
    }

    void makeQuad()
    {
        vertices = {
            Vertex{vec3(-1, -1, 0.0), vec3(1),  vec4(1), vec2(0, 0)},
            Vertex{vec3(-1, 1, 0.0),  vec3(1),  vec4(1), vec2(0, 1)},
            Vertex{vec3(1, 1, 0.0),   vec3(1),  vec4(1), vec2(1, 1)},
            Vertex{vec3(1, -1, 0.0), vec3(1), vec4(1), vec2(1, 0)}
        };
        indices = {0, 1, 2, 2, 3, 0};    
    }
};
REGISTER_COMPONENT(Geometry)

struct Mesh
{
    std::vector<Handle> geometries;
};
REGISTER_COMPONENT(Mesh)

struct Renderable
{
    Handle handle;
    int layer = 1;
};
REGISTER_COMPONENT(Renderable)

struct Pipeline
{
    std::string fileName;    
    bool screenSpace = false;
    int layers=1;
};
REGISTER_COMPONENT(Pipeline)

struct GLTexture
{
    Texture info;
    GLuint textureName;
};
REGISTER_COMPONENT(GLTexture)

struct GLPipeline
{
    Pipeline info;
    GLuint programName;
    short uniforms[16];
    short attachments[16];
    short attributes[16];
};
REGISTER_COMPONENT(GLPipeline)

struct GLRenderTarget
{
    RenderTarget info;
    ivec2 size;?
    bool multisampled = false;
    GLuint framebufferName = 0;
    GLuint blitFrameBufferName = 0;
    GLuint depthrenderbufferName = 0;

    GLTexture texture;
    GLTexture blitTexture;
};
REGISTER_COMPONENT(GLRenderTarget)

struct GLFrameBuffer
{    
    ivec2 size;
    GLuint framebufferName = 0;
    GLbitfield clearMask;
};
REGISTER_COMPONENT(GLFrameBuffer)

enum class AttachmentType
{
    Color,
    Depth
};

struct Attachment
{
    RenderDescriptorType name;
    AttachmentType type;
};
REGISTER_COMPONENT(Attachment)

struct Subpass
{
    std::vector<RenderDescriptorType> inputs;
    std::vector<RenderDescriptorType> outputs;
    int clearMask = 0xFFF;
    Pipeline pipeline;
};
REGISTER_COMPONENT(Subpass)

struct RenderPass
{
    ivec2 size;
    std::vector<Attachment> attachments;
    std::vector<Subpass> subpasses;
    std::vector<Handle> nodes;
};
REGISTER_COMPONENT(RenderPass)

struct SubpassInstance
{
    GLPipeline glPipeline;
    GLFrameBuffer glFrameBuffer;
};
REGISTER_COMPONENT(SubpassInstance)

struct GLAttachment
{
    Attachment info;
    GLTexture texture;    
};
REGISTER_COMPONENT(GLAttachment)

struct RenderPassInstance
{
    RenderPass renderPass;
    std::vector<GLAttachment> attachments;
    std::vector<SubpassInstance> subpasses;
}; 
REGISTER_COMPONENT(RenderPassInstance)

struct GLGeometry
{
    bool useIndex;
    GLuint vertexbuffer;
    GLuint indexbuffer;
    GLuint vao;
    GLenum type;
    GLuint count;
};
REGISTER_COMPONENT(GLGeometry)

struct Material
{
    enum Type
    {
        Simple,
        PBR
    } type;
    Color diffuseColor;
    std::vector<Handle> textures;
};
REGISTER_COMPONENT(Material)

#endif // render_h__