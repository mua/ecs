#include "systems/ResourceSys.h"

#include "components/render.h"
#include "engine.h"

#include <gl/glew.h>
#include <loguru.hpp>

#include <json.hpp>
using json = nlohmann::json;

#include <filesystem>

#ifdef EMSCRIPTEN
std::string getExePath()
{
    return "";
}
#else

#include <filesystem>
#include <windows.h>

std::string getExePath()
{
    char result[MAX_PATH];
    GetModuleFileName(NULL, result, MAX_PATH);
    auto path = std::filesystem::path(result);
    return path.parent_path().string();
}

#endif // EMSCRIPTEN

#include "systems/ResourceSys.h"
#include <fstream>
#include <sstream>
#include "components/core.h"

std::vector<std::string> ResourceSys::searchPaths;
ResourceSys *ResourceSys::_instance = nullptr;

GLTexture createTexture(Texture info, GLuint name = 0, int samples = 1)
{
    GLTexture ret;
    ret.info = info;
    ret.textureName = name;
    if (name == 0)
        glGenTextures(1, &ret.textureName);
    if (samples == 1)
    {
        glBindTexture(GL_TEXTURE_2D, ret.textureName);
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.textureName);
    }

    switch (info.type)
    {
    case Texture::Type::Depth:
        if (samples == 1)
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, info.size.x, info.size.y, 0, GL_DEPTH_COMPONENT,
                         GL_FLOAT, 0);
        }
        else
        {
            //glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_DEPTH_COMPONENT, info.size.x, info.size.y,
            //                        GL_TRUE);
        }
        break;
    default:
        if (info.pixels.size() == 0)
        {
            if (samples == 1)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, info.size.x, info.size.y, 0, GL_RGBA, GL_FLOAT, 0);
            }
            else
            {
                //glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, info.size.x, info.size.y, GL_TRUE);
            }
        }
        else
        {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, info.size.x, info.size.y, 0, GL_RGBA, GL_FLOAT, &info.pixels[0]);
            //glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, info.size.x, info.size.y, GL_TRUE);
        }
        // glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        break;
    }

    switch (info.wrap)
    {
    case Texture::Wrap::Clamp:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        break;
    default:
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        break;
    }
    LOG_F(INFO, "Texture created");
    return ret;
}

GLTexture createTextureMultisample(Texture info, int samples = 4)
{
    GLTexture ret;
    glGenTextures(1, &ret.textureName);
    glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, ret.textureName);
    glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, GL_RGB, info.size.x, info.size.y, GL_TRUE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    return ret;
}

void destroyTexture(GLTexture texture)
{
    glDeleteTextures(1, &texture.textureName);
}

GLFrameBuffer createFrameBuffer(ivec2 size, std::vector<GLAttachment> attachments, int clearMask)
{
    GLFrameBuffer ret;
    ret.size = size;
    ret.clearMask = 0;
    glGenFramebuffers(1, &ret.framebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, ret.framebufferName);
    int i = 0;
    int attachmentIndex = 0;
    std::vector<GLenum> drawBuffers;
    for (auto attachment : attachments)
    {
        if (attachment.info.type == AttachmentType::Color)
        {
            if (clearMask & 1<<attachmentIndex)
                ret.clearMask |= GL_COLOR_BUFFER_BIT;
            auto buf = GL_COLOR_ATTACHMENT0 + i;
            glFramebufferTexture2D(GL_FRAMEBUFFER, buf, GL_TEXTURE_2D, attachment.texture.textureName, 0);
            drawBuffers.push_back(buf);
            i++;
        }
        if (attachment.info.type == AttachmentType::Depth)
        {
            if (clearMask & 1<<attachmentIndex)
                ret.clearMask |= GL_DEPTH_BUFFER_BIT;
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, attachment.texture.textureName,
                                   0);
        }
        attachmentIndex++;
    }
    glDrawBuffers(drawBuffers.size(), drawBuffers.data());
    auto fs = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(fs == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ret;
}

GLRenderTarget createRenderTarget(RenderTarget info, bool depth = true)
{
    GLRenderTarget ret;
    ret.info = info;
    ret.texture = createTexture(Texture{ret.info.size});
    ret.size = info.size;
    glGenFramebuffers(1, &ret.framebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, ret.framebufferName);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ret.texture.textureName, 0);
    if (depth)
    {
        glGenRenderbuffers(1, &ret.depthrenderbufferName);
        glBindRenderbuffer(GL_RENDERBUFFER, ret.depthrenderbufferName);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, info.size.x, info.size.y);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ret.depthrenderbufferName);
    }
    // 		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
    // texture.texture, 0); 		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
    // 		glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    auto fs = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    assert(fs == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ret;
}

GLRenderTarget createRenderTargetMultisampled(RenderTarget info)
{
    GLRenderTarget ret;
    ret.info = info;
    ret.texture = createTextureMultisample(Texture{ret.info.size}, 4);
    ret.size = info.size;
    glGenFramebuffers(1, &ret.framebufferName);
    glBindFramebuffer(GL_FRAMEBUFFER, ret.framebufferName);
    glGenRenderbuffers(1, &ret.depthrenderbufferName);
    glBindRenderbuffer(GL_RENDERBUFFER, ret.depthrenderbufferName);
    glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT16, info.size.x, info.size.y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, ret.depthrenderbufferName);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, ret.texture.textureName, 0);
    ret.multisampled = true;
    auto mrt = createRenderTarget(info, false);
    ret.blitFrameBufferName = mrt.framebufferName;
    ret.blitTexture = mrt.texture;
    return ret;
}

void destroyRenderTarget(GLRenderTarget target)
{
    glDeleteFramebuffers(1, &target.framebufferName);
    glDeleteRenderbuffers(1, &target.depthrenderbufferName);
    destroyTexture(target.texture);
}

GLuint compileShaderFromSource(std::string source, GLuint shaderType)
{
    auto shader = glCreateShader(shaderType);
    const char *src = source.c_str();

    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    // LOG_F(INFO, "Compiling: \n%s", src);
    GLint status = GL_FALSE;
    GLint infoLength;
    std::vector<char> message;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

    if (infoLength)
    {
        message.resize(size_t(infoLength) + 1);
        glGetShaderInfoLog(shader, infoLength, NULL, message.data());
    }
    if (status != GL_TRUE)
    {
        LOG_F(ERROR, "Shader Compilation Failed: \n %s", message.data());
        return 0;
    }
    else if (infoLength)
    {
        LOG_F(WARNING, "Shader Compilation warning: \n %s", message.data());
    }
    return shader;
}

GLuint compileShader(std::string _fileName, GLuint shaderType)
{
    auto fileName = ResourceSys::absPath(_fileName);

    std::ifstream is(fileName);
    std::stringstream source;
    if (!is.fail())
    {
        source << is.rdbuf();
    }
    else
    {
        LOG_F(ERROR, "Cannot load shader %s", _fileName.c_str());
        return 0;
    }

    if (auto shader = compileShaderFromSource(source.str(), shaderType))
    {
        LOG_F(INFO, "Shader Compilation Complete: %s", _fileName.c_str());
        return shader;
    }
    return 0;
}

GLuint linkProgram(GLuint vertex, GLuint fragment)
{
    GLuint programName = glCreateProgram();
    glAttachShader(programName, vertex);
    glAttachShader(programName, fragment);
    glLinkProgram(programName);

    GLint infoLogLen;
    int success;
    glGetProgramiv(programName, GL_LINK_STATUS, &success);
    glGetProgramiv(programName, GL_INFO_LOG_LENGTH, &infoLogLen);
    if (infoLogLen > 0)
    {
        std::vector<char> message(size_t(infoLogLen) + 1);
        glGetProgramInfoLog(programName, infoLogLen, NULL, &message[0]);
        LOG_F(ERROR, "Shader link error:\n%s\n", &message[0]);
    }
    glDetachShader(programName, vertex);
    glDetachShader(programName, fragment);

    glDeleteShader(vertex);
    glDeleteShader(fragment);

    return programName;
}

RenderDescriptorType toRenderDescType(std::string name)
{

    for (int i = 0; RenderDescriptorTypeNames[i]; i++)
    {
        auto dname = RenderDescriptorTypeNames[i];
        if (!strcmp(name.c_str(), dname))
            return RenderDescriptorType(i + 1);
    }
    return UNKNOWN_TYPE;
}

GLPipeline ResourceSys::createPipeline(Pipeline info)
{
    GLPipeline *gp = 0;
    r->each<Pipeline, GLPipeline>([&](int id, Pipeline &pipe, GLPipeline &gpipe) {
        if (info.fileName == pipe.fileName)
        {
            gp = &gpipe;
        }
    });
    if (gp)
    {
        auto pl = *gp;
        pl.info = info;
        return pl;
    }

    GLPipeline pipeline;
    pipeline.info = info;
    auto vertex = compileShader(info.fileName + ".vert", GL_VERTEX_SHADER);
    auto fragment = compileShader(info.fileName + ".frag", GL_FRAGMENT_SHADER);
    if (vertex && fragment)
    {
        pipeline.programName = linkProgram(vertex, fragment);
    }
    std::ifstream fs(ResourceSys::absPath(info.fileName + ".json"));
    json js;
    fs >> js;
    int i = 0;
    for (auto var : js["uniforms"])
    {
        auto name = var["name"].get<std::string>();
        auto type = toRenderDescType(name);
        if (type == UNKNOWN_TYPE)
        {
            LOG_F(ERROR, "Shader param is not recognized ! %s", name.c_str());
        }
        pipeline.uniforms[type] = glGetUniformLocation(pipeline.programName, name.c_str());
        i++;
    }
    r->createEntity(info, pipeline, Info{"Pipeline:"+info.fileName, 1});
    return pipeline;
}

void createSubpassFramebuffers(RenderPassInstance &instance)
{
    std::map<RenderDescriptorType, GLAttachment> attachmentMap;
    for (auto glAttachment : instance.attachments)
    {
        attachmentMap[glAttachment.info.name] = glAttachment;
    }
    for (int i = 0; i < instance.renderPass.subpasses.size(); i++)
    {
        std::vector<GLAttachment> attachments;
        for (auto out : instance.renderPass.subpasses[i].outputs)
        {
            attachments.push_back(attachmentMap[out]);
        }
        instance.subpasses[i].glFrameBuffer = createFrameBuffer(instance.renderPass.size, attachments, instance.renderPass.subpasses[i].clearMask);
    }
}

RenderPassInstance ResourceSys::createRenderPass(RenderPass info)
{
    RenderPassInstance instance;
    instance.renderPass = info;

    for (auto attachment : info.attachments)
    {
        GLAttachment glAttachment;
        auto tex = Texture{info.size};
        switch (attachment.type)
        {
        case AttachmentType::Color:
            tex.type = Texture::Type::RGBA;
            break;
        case AttachmentType::Depth:
            tex.type = Texture::Type::Depth;
            break;
        default:
            LOG_F(ERROR, "Unrecognized attachment type");
        }
        glAttachment.texture = createTexture(tex);
        glAttachment.info = attachment;
        instance.attachments.push_back(glAttachment);
    }

    instance.subpasses.resize(info.subpasses.size());
    for (int i = 0; i < info.subpasses.size(); i++)
    {
        instance.subpasses[i].glPipeline = createPipeline(info.subpasses[i].pipeline);
    }
    createSubpassFramebuffers(instance);
    return instance;
}

void resizeRenderPass(RenderPass info, RenderPassInstance &instance)
{
    instance.renderPass = info;
    for (int i = 0; i < instance.attachments.size(); i++)
    {
        Texture texInfo = instance.attachments[i].texture.info;
        texInfo.size = info.size;
        instance.attachments[i].texture = createTexture(texInfo, instance.attachments[i].texture.textureName);
    }
    for (auto sub : instance.subpasses)
    {
        glDeleteFramebuffers(1, &sub.glFrameBuffer.framebufferName);
    }
    LOG_F(INFO, "Resizing renderpass");
    createSubpassFramebuffers(instance);
}

void destroyRenderPass(RenderPassInstance &ins)
{
    //     for (int i = 0; i < ins.subpassCount; i++)
    //     {
    //         destroyRenderTarget(ins.subpasses[i].target);
    //     }
}

GLGeometry createGeometry(Geometry info)
{
    GLGeometry geo;
    geo.indexbuffer = 0;
    geo.vertexbuffer = 0;

    glGenVertexArrays(1, &geo.vao);
    glBindVertexArray(geo.vao);

    glGenBuffers(1, &geo.vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, geo.vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, info.vertices.size() * sizeof(Vertex), info.vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3,
                          GL_FLOAT,                     // type
                          GL_FALSE,                     // normalized?
                          sizeof(Vertex),               // stride
                          (void *)offsetof(Vertex, pos) // array buffer offset
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 4,
                          GL_FLOAT,                       // type
                          GL_FALSE,                       // normalized?
                          sizeof(Vertex),                 // stride
                          (void *)offsetof(Vertex, color) // array buffer offset
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2,
                          GL_FLOAT,                    // type
                          GL_FALSE,                    // normalized?
                          sizeof(Vertex),              // stride
                          (void *)offsetof(Vertex, uv) // array buffer offset
    );
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 3,
                          GL_FLOAT,                        // type
                          GL_FALSE,                        // normalized?
                          sizeof(Vertex),                  // stride
                          (void *)offsetof(Vertex, normal) // array buffer offset
    );
    glEnableVertexAttribArray(3);

    glGenBuffers(1, &geo.indexbuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo.indexbuffer);

    if (info.indices.size())
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, info.indices.size() * sizeof(unsigned int), &info.indices[0],
                     GL_STATIC_DRAW);
        geo.count = info.indices.size();
        geo.useIndex = true;
    }
    else
    {        
        geo.count = info.vertices.size();
        geo.useIndex = false;
    }

    glBindVertexArray(0);

    switch (info.type)
    {
    case Geometry::GeometryType::Lines:
        geo.type = GL_LINES;
        break;
    case Geometry::GeometryType::Triangles:
        geo.type = GL_TRIANGLES;
        break;
    }
    return geo;
}

void updateGeometry(Geometry &info, GLGeometry &geo)
{
    glBindBuffer(GL_ARRAY_BUFFER, geo.vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, info.vertices.size() * sizeof(Vertex), info.vertices.data(), GL_STATIC_DRAW);
    if (info.indices.size())
    {
        if (!geo.indexbuffer)
        {
            glGenBuffers(1, &geo.indexbuffer);
        }
        geo.useIndex = true;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geo.indexbuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, info.indices.size() * sizeof(unsigned int), &info.indices[0],
                     GL_STATIC_DRAW);
        geo.count = info.indices.size();
    }
    else
    {
        geo.count = info.vertices.size();
    }
    info.updated = false;
}

ResourceSys::ResourceSys()
{
    auto base = std::filesystem::path(getExePath());
    searchPaths.push_back((base / std::filesystem::path("_resources")).string());
    searchPaths.push_back((base / std::filesystem::path("resources")).string());
}

ResourceSys::~ResourceSys()
{
}

void ResourceSys::start()
{
}

void ResourceSys::process()
{
    //     syncResource<RenderTarget, GLRenderTarget>(
    //         [](Handle entity, auto info) { return createRenderTargetMultisampled(info); },
    //         [](RenderTarget &target) { return target.size.x > 0 && target.size.y > 0; });
    syncResource<RenderPass, RenderPassInstance>([&](Handle entity, auto info) { return createRenderPass(info); },
                                                 [](RenderPass &pass) { return pass.size.x > 0 && pass.size.y > 0; });
    syncResource<Geometry, GLGeometry>(
        [](Handle entity, auto &info) {
            info.updated = false;
            return createGeometry(info);
        },
        nullptr, [&](Geometry &geo, GLGeometry& ggeo) {  
            if (geo.updated)
                updateGeometry(geo, ggeo);
        });

    syncResource<Texture, GLTexture>([](Handle entity, auto info) { return createTexture(info); });

    r->each<RenderPass, RenderPassInstance>(
        [&](int entity, RenderPass &pass, RenderPassInstance &ins) {
            if (pass.size != ins.renderPass.size)
            {
                resizeRenderPass(pass, ins);
            }
        });
}

std::string ResourceSys::absPath(std::string path)
{
    auto filePath = std::filesystem::path(path);
    for (auto sp : searchPaths)
    {
        auto dirPath = std::filesystem::path(sp);
        auto jpath = dirPath / filePath;
        if (std::filesystem::exists(jpath))
            return jpath.string();
    }
    return "";
}

ResourceSys *ResourceSys::instance()
{
    if (!_instance)
    {
        _instance = new ResourceSys();
    }
    return _instance;
}

