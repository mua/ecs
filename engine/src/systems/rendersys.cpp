#include "systems/RenderSys.h"
#include "Engine.h"
#include "components/Render.h"
#include "components/transform.h"

#include <gl/GLU.h>
#include <gl/glew.h>

#include <loguru.hpp>

#include <random>
#include "components/core.h"

const char *RenderDescriptorTypeNames[] = {
    "model", "view", "projection", "diffuseColor", "baseColorTexture", "ssaoKernel", "ssaoNoiseTexture", "mapColor", "mapNormal", "mapDepth", "mapPosition", "mapSSAO", 0,
};

RenderSys *RenderSys::_instance = nullptr;
Handle quadHandle, ssaoNoiseTextureHandle;
std::vector<glm::vec3> ssaoKernel;

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
                                const GLchar *message, const void *userParam)
{
    LOG_F(ERROR, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
          (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""), type, severity, message);
}

void RenderSys::bindMaterial(const Geometry &geo, const GLPipeline &pipeline)
{
    if (geo.material)
    {
        auto &[mat] = r->getEntity<Material>(geo.material);
        glUniform4fv(pipeline.uniforms[RenderDescriptorType::MATERIAL_DIFFUSE], 1,
                     glm::value_ptr((vec4)mat.diffuseColor));
        for (size_t i = 0; i < mat.textures.size(); i++)
        {
            auto &[tex] = r->getEntity<GLTexture>(mat.textures[i]);
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, tex.textureName);
            glUniform1i(pipeline.uniforms[RenderDescriptorType::MATERIAL_BASE_COLOR_TEXTURE], i);
        }
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, 0);
        glUniform4fv(pipeline.uniforms[RenderDescriptorType::MATERIAL_DIFFUSE], 1, glm::value_ptr(vec4(1.0)));
    }
}

void RenderSys::drawGeometry(const GLPipeline &pipeline, const GLGeometry &ggeo)
{
    glBindVertexArray(ggeo.vao);
    if (ggeo.useIndex)
    {
        glDrawElements(ggeo.type,       // mode
                       ggeo.count,      // count
                       GL_UNSIGNED_INT, // type
                       (void *)0        // element array buffer offset
        );
    }
    else
    {
        glDrawArrays(ggeo.type, 0, ggeo.count);
    }
}

void RenderSys::renderToTarget(GLFrameBuffer &target, GLPipeline& pipeline, Camera &camera,
                               std::vector<GLAttachment> &attachments)
{
    glBindFramebuffer(GL_FRAMEBUFFER, target.framebufferName);
    glViewport(0, 0, target.size.x, target.size.y);
    glClearColor(0.010f, 0.050f, 0.070f, 1.000f);
    glClear(target.clearMask);
    
    glUseProgram(pipeline.programName);
    glUniformMatrix4fv(pipeline.uniforms[RenderDescriptorType::TRANSFORM_VIEW], 1, GL_FALSE,
                       glm::value_ptr(camera.view));
    glUniformMatrix4fv(pipeline.uniforms[RenderDescriptorType::TRANSFORM_PROJECTION], 1, GL_FALSE,
                       glm::value_ptr(camera.projection));


    int i = 2;
    for (auto &attachment: attachments)
    {
        if (pipeline.uniforms[attachment.info.name] > -1)
        {
            i++;
            glActiveTexture(GL_TEXTURE0 + i);
            glBindTexture(GL_TEXTURE_2D, attachment.texture.textureName);
            glUniform1i(pipeline.uniforms[attachment.info.name], i);             
        }
    }    

    if (pipeline.info.screenSpace)
    {
        GLGeometry ggeo;
        if (r->get(quadHandle, ggeo))
        {
            GLTexture tex;
            if (r->get(ssaoNoiseTextureHandle, tex))
            {
                i++;
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, tex.textureName);
                auto loc = glGetUniformLocation(pipeline.programName, "ssaoNoiseTexture");
                glUniform1i(loc, i);
            }
            glUniform3fv(glGetUniformLocation(pipeline.programName, "ssaoKernel"), ssaoKernel.size(), (GLfloat*)ssaoKernel.data());
            drawGeometry(pipeline, ggeo);
        }
    }
    else
    {
        r->each<Transform, Geometry, GLGeometry>(
            [&](int entity, Transform &transform, Geometry &geo, GLGeometry &ggeo) {
                auto inf = r->getPtr<Info>(entity);
                if (!(geo.layer & pipeline.info.layers) || (inf && !inf->active))
                    return;
                glUniformMatrix4fv(pipeline.uniforms[RenderDescriptorType::TRANSFORM_MODEL], 1, GL_FALSE,
                                   glm::value_ptr(transform.worldMatrix()));
                bindMaterial(geo, pipeline);
                drawGeometry(pipeline, ggeo);
            });

        r->each<Transform, Renderable>(
            [&](int entity, Transform &transform, Renderable& renderable) {
                auto inf = r->getPtr<Info>(entity);
                if (!(renderable.layer & pipeline.info.layers) || (inf && !inf->active))
                    return;
                glUniformMatrix4fv(pipeline.uniforms[RenderDescriptorType::TRANSFORM_MODEL], 1, GL_FALSE,
                                   glm::value_ptr(transform.worldMatrix()));
                auto &[mesh] = r->getEntity<Mesh>(renderable.handle);
                for (auto geoHandle : mesh.geometries)
                {
                    auto &[geo, ggeo] = r->getEntity<Geometry, GLGeometry>(geoHandle);
                    bindMaterial(geo, pipeline);
                    drawGeometry(pipeline, ggeo);
                }
            });
    }
    //     if (target.multisampled)
//     {
//         glBindFramebuffer(GL_READ_FRAMEBUFFER, target.framebufferName);
//         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, target.blitFrameBufferName);
//         glBlitFramebuffer(0, 0, target.size.x, target.size.y, 0, 0, target.size.x, target.size.y, GL_COLOR_BUFFER_BIT,
//                           GL_NEAREST);
//         glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
//     }

    for (;i > -1; --i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderSys::renderPass(RenderPassInstance &pass, Camera &camera)
{    
    for (size_t i = 0; i < pass.subpasses.size(); i++)
    {
        renderToTarget(pass.subpasses[i].glFrameBuffer, pass.subpasses[i].glPipeline, camera, pass.attachments);
    }
}

RenderSys::RenderSys()
{


}

RenderSys::~RenderSys()
{
}

void RenderSys::start()
{
    Geometry geo;
    geo.makeQuad();
    quadHandle = r->createEntity(geo, Info{"Quad", 1});

    // generate sample kernel
    // ----------------------
    std::uniform_real_distribution<GLfloat> randomFloats(0.0, 1.0); // generates random floats between 0.0 and 1.0
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0,
                         randomFloats(generator));
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0;

        // scale samples s.t. they're more aligned to center of kernel
        scale = 0.1f + 0.9f * scale * scale;
        sample *= scale;
        ssaoKernel.push_back(sample);
    }

    // generate noise texture
    // ----------------------
    Texture tex;
    tex.wrap = Texture::Wrap::Repeat;
    tex.size = ivec2(4, 4);
    std::vector<glm::vec4> pixels;
    for (unsigned int i = 0; i < 16; i++)
    {
        glm::vec4 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f,
                        0.0f); // rotate around z-axis (in tangent space)
        pixels.push_back(noise);
    }
    tex.pixels.resize(pixels.size() * sizeof(float) * 4);
    memcpy(tex.pixels.data(), pixels.data(), tex.pixels.size());
    ssaoNoiseTextureHandle = r->createEntity(tex, Info{"SSAONoise", 1});
#ifndef EMSCRIPTEN
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(MessageCallback, 0);
#endif // !EMSCRIPTEN
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);  
    glDepthFunc(GL_LESS); 

    glLineWidth(1.5f);
}

void RenderSys::process()
{
    r->each<RenderPassInstance, Camera>(
        [&](int entity, auto &pass, auto &camera) { renderPass(pass, camera); });
}

void *RenderSys::uiRenderTargetHandle(Handle entity)
{
    RenderPassInstance ins;
    if (Engine::instance()->registry.get(entity, ins))
    {
        //auto name = !glTarget.multisampled ? glTarget.texture.textureName : glTarget.blitTexture.textureName;
        return (void *)(intptr_t)ins.attachments.front().texture.textureName;
    }
    return nullptr;
}

RenderSys *RenderSys::instance()
{
    if (!_instance)
    {
        _instance = new RenderSys();
    }
    return _instance;
}
