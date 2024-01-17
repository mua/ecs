#include "systems/loadsys.h"

#include "components/core.h"
#include "engine.h"

#include <filesystem>

#include "loguru.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "components/Render.h"
#include "tiny_gltf.h"
#include "components/transform.h"

Handle instantiate(Registry *r, Handle source, Handle target);

LoadSys::LoadSys()
{
}

LoadSys::~LoadSys()
{
}

void LoadSys::start()
{
}

void LoadSys::process()
{
    syncResource<Ref, Instance>([&](Handle entity, Ref& ref) {
        auto ext = std::filesystem::path(ref.path).extension().string();
        if (ext == ".gltf" || ext == ".glb")
        {
            bool loaded = false;
            Handle protoHandle = 0;
            r->each<Proto>([&](Handle _handle, auto proto) {
                if (proto.path == ref.path)
                {
                    protoHandle = _handle;
                    loaded = true;
                }
            });
            if (!loaded)
            {
                protoHandle = loadGLTF(ref.path);
                r->addComponent(protoHandle, Proto());
            }
            instantiate(r, protoHandle, entity);
            r->get<Info>(entity).name = "Instance:" + ref.path;
        }
        return Instance();
    });
}

Handle instantiate(Registry *r, Handle source, Handle target)
{
    assert(target);
    r->copy(source, target);
    r->addComponent(target, Instance());
    r->removeComponent<Proto>(target);
    r->get<Info>(target).active = true;
    r->get<Info>(source).active = false;
    Relation rel, newRel;
    if (r->get(target, rel))
    {
        newRel.parent = rel.parent;
        for (auto child: rel.children)
        {
            auto cpyChild = r->nextEntityId();
            instantiate(r, child, cpyChild);
            newRel.children.push_back(cpyChild);
            r->get<Relation>(cpyChild).parent = target;
        }
        rel = newRel;
        r->addComponent(target, rel);
    }
    return target;
}

void openModel(std::string &fileName, tinygltf::Model &model);
std::vector<Handle> importTextures(Registry *r, tinygltf::Model &model);
std::vector<Handle> importMaterials(Registry *r, tinygltf::Model &model, std::vector<Handle> &textures);
std::map<int, Handle> importMeshes(Registry *r, tinygltf::Model &model, std::vector<Handle> &materials);
std::vector<Handle> importNodes(Registry *r, tinygltf::Model &model, std::map<int, Handle> &meshes);

void addChildren(Registry *r, Handle parent, std::vector<Handle> &children)
{
    Relation &rel = r->get<Relation>(parent); 
    for (auto child: children)
    {
        auto &childRel = r->get<Relation>(child);
        if (!childRel.parent)
        {
            childRel.parent = parent;
            rel.add(child);
        }
    }
}

Handle LoadSys::loadGLTF(std::string path)
{
    tinygltf::Model model;
    openModel(path, model);
    int i = 0;
    for (auto &node : model.nodes)
    {
        i++;
        if (node.name.empty())
        {
            node.name = std::string("Node_") + std::to_string(i++);
        }
    }

    auto textures = importTextures(r, model);
    auto materials = importMaterials(r, model, textures);
    auto meshMap = importMeshes(r, model, materials);    
    auto nodes = importNodes(r, model, meshMap);

    auto root = r->createEntity(Transform());    
    auto rootRel = Relation();
    for (auto node : nodes)
    {
        auto& [rel] = r->getEntity<Relation>(node);
        if (rel.parent == 0)
        {
            rootRel.add(node);
            rel.parent = root;
        }
    }
    r->addComponent(root, rootRel);
    r->addComponent(root, Info{"Proto:" + path, false});

    std::vector<Handle> meshes;
    for (auto mv: meshMap)
    {
        meshes.push_back(mv.second);
    }
    addChildren(r, root, textures);
    addChildren(r, root, materials);
    addChildren(r, root, meshes);

    return root;
}

void openModel(std::string &fileName, tinygltf::Model &model)
{
    tinygltf::TinyGLTF gltf_ctx;
    std::string err;
    std::string warn;
    std::string ext = std::filesystem::path(fileName).extension().string();

    bool ret = false;
    if (ext.compare(".glb") == 0)
    {
        LOG_F(INFO, "Reading binary glTF");
        ret = gltf_ctx.LoadBinaryFromFile(&model, &err, &warn, fileName.c_str());
    }
    else
    {
        LOG_F(INFO, "Reading ASCII glTF");
        ret = gltf_ctx.LoadASCIIFromFile(&model, &err, &warn, fileName.c_str());
    }

    if (!warn.empty())
    {
        LOG_F(WARNING, "Warn: %s\n", warn.c_str());
    }

    if (!err.empty())
    {
        LOG_F(ERROR, "Err: %s\n", err.c_str());
    }

    if (!ret)
    {
        LOG_F(ERROR, "Failed to parse glTF\n");
    }
}

std::map<int, Handle> importMeshes(Registry* r, tinygltf::Model &model, std::vector<Handle> &materials)
{
    std::map<int, Handle> ret;

    int i = 0;
    for (auto mesh : model.meshes)
    {
        auto entity = r->createEntity(Mesh());
        Mesh &meshComp = r->get<Mesh>(entity);
        ret[i++] = entity;

        for (auto primitive : mesh.primitives)
        {
            float *posBuffer = nullptr;
            float *texBuffer = nullptr;
            float *normBuffer = nullptr;
            uint16_t *jointsBuffer = nullptr;
            float *weightsBuffer = nullptr;

            size_t posStride = 3, normStride = 3, texStride = 2, jointsStride = 4, weightsStride = 4;

            if (!primitive.attributes.count("POSITION"))
            {
                throw std::runtime_error{"GLTF import error: positions are required"};
            }
            int pos = primitive.attributes["POSITION"];
            auto posAcc = model.accessors[pos];
            auto posView = model.bufferViews[posAcc.bufferView];
            posBuffer = (float *)&model.buffers[posView.buffer].data[posAcc.byteOffset + posView.byteOffset];
            posStride = posView.byteStride ? posView.byteStride / 4 : posStride;
            int vertexCount = int(posAcc.count);

            if (primitive.attributes.count("NORMAL"))
            {
                int norm = primitive.attributes["NORMAL"];
                auto normAcc = model.accessors[norm];
                auto normView = model.bufferViews[normAcc.bufferView];
                normStride = normView.byteStride ? normView.byteStride / 4 : normStride;
                normBuffer = (float *)&model.buffers[normView.buffer].data[normAcc.byteOffset + normView.byteOffset];
            }

            if (primitive.attributes.count("JOINTS_0"))
            {
                auto acc = model.accessors[primitive.attributes["JOINTS_0"]];
                auto view = model.bufferViews[acc.bufferView];
                jointsStride = view.byteStride ? view.byteStride / sizeof(uint16_t)
                                               : (tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4));
                jointsBuffer = (uint16_t *)&model.buffers[view.buffer].data[acc.byteOffset + view.byteOffset];
            }

            if (primitive.attributes.count("WEIGHTS_0"))
            {
                auto acc = model.accessors[primitive.attributes["WEIGHTS_0"]];
                auto view = model.bufferViews[acc.bufferView];
                weightsStride = view.byteStride ? view.byteStride / sizeof(float)
                                                : (tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4));
                weightsBuffer = (float *)&model.buffers[view.buffer].data[acc.byteOffset + view.byteOffset];
            }

            if (primitive.attributes.count("TEXCOORD_0"))
            {
                int tex = primitive.attributes["TEXCOORD_0"];
                auto texAcc = model.accessors[tex];
                auto texView = model.bufferViews[texAcc.bufferView];
                texStride = texView.byteStride ? texView.byteStride / 4 : texStride;
                texBuffer = (float *)&model.buffers[texView.buffer].data[texAcc.byteOffset + texView.byteOffset];
            }

            std::vector<uint32_t> indices;
            if (primitive.indices > -1)
            {
                auto indexAcc = model.accessors[primitive.indices];
                auto indexView = model.bufferViews[indexAcc.bufferView];
                auto indexBuffer = model.buffers[indexView.buffer];
                auto data = &indexBuffer.data[indexAcc.byteOffset + indexView.byteOffset];
                for (int i = 0; i < indexAcc.count; i++)
                {
                    switch (indexAcc.componentType)
                    {
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT:
                        indices.push_back(((uint32_t *)data)[i]);
                        break;
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT:
                        indices.push_back(((uint16_t *)data)[i]);
                        break;
                    case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE:
                        indices.push_back(((uint8_t *)data)[i]);
                        break;
                    }
                }
            }
            Geometry geo;
            geo.vertices.resize(vertexCount);
            //             if (jointsBuffer)
            //             {
            //                 mesh->skinBuffer.vertices.resize(vertexCount);
            //             }
            for (int i = 0; i < vertexCount; i++)
            {
                Vertex vert;
                vert.color = Color::white;
                vert.pos = glm::make_vec3(&posBuffer[i * posStride]);
                if (normBuffer)
                {
                    vert.normal = glm::make_vec3(&normBuffer[i * normStride]);
                }
                if (texBuffer)
                {
                    vert.uv = glm::make_vec2(&texBuffer[i * texStride]);
                }
                //                 if (jointsBuffer)
                //                 {
                //                     mesh->skinBuffer.vertices[i].weights = glm::make_vec4(&weightsBuffer[i *
                //                     weightsStride]); mesh->skinBuffer.vertices[i].bones.x = jointsBuffer[i *
                //                     jointsStride + 0]; mesh->skinBuffer.vertices[i].bones.y = jointsBuffer[i *
                //                     jointsStride + 1]; mesh->skinBuffer.vertices[i].bones.z = jointsBuffer[i *
                //                     jointsStride + 2]; mesh->skinBuffer.vertices[i].bones.w = jointsBuffer[i *
                //                     jointsStride + 3];
                //                 }
                geo.vertices[i] = vert;
            }
            if (indices.size())
            {
                geo.indices = indices;
            }
            if (primitive.material > -1)
            {
                geo.material = materials[primitive.material];
            }
            else
            {
                geo.material = 0;
            }
            geo.updateBBox();
            auto geoEntity = r->createEntity(geo);
            meshComp.geometries.push_back(geoEntity);
            auto children = std::vector<Handle>{geoEntity};
            addChildren(r, entity, children);
        }

        LOG_F(INFO, "Imported %d primitives", mesh.primitives.size());
    }
    return ret;
}

std::vector<Handle> importTextures(Registry *r, tinygltf::Model &model)
{
    std::vector<Handle> ret;
    for (auto tex : model.textures)
    {
        auto image = model.images[tex.source];
        auto texture = Texture{ivec2(image.width, image.height)};
        if (image.component == 3)
        {
            texture.pixels.resize(image.width * image.height * 4);
            for (int i = 0; i < image.image.size() / 3; i++)
            {
                texture.pixels[i * 4 + 0] = image.image[i * 3 + 0];
                texture.pixels[i * 4 + 1] = image.image[i * 3 + 1];
                texture.pixels[i * 4 + 2] = image.image[i * 3 + 2];
                texture.pixels[i * 4 + 3] = 255;
            }
        }
        else if (image.component == 4)
        {
            texture.pixels = image.image;
        }
        else
        {
            throw std::runtime_error("GLTF Import Error: unsupported texture type");
        }
        ret.push_back(r->createEntity(texture));
    }    
    return ret;
}

std::vector<Handle> importMaterials(Registry *r, tinygltf::Model &model, std::vector<Handle> &textures)
{
    auto ret = std::vector<Handle>();
    for (auto gmat : model.materials)
    {
        Material material;
        material.diffuseColor = vec4(1.0);
        if (gmat.pbrMetallicRoughness.metallicRoughnessTexture.index == -1)
            material.type = Material::Simple;
        else
            material.type = Material::PBR;
        if (gmat.values.count("baseColorTexture"))
        {
            material.textures.push_back(textures[gmat.values["baseColorTexture"].TextureIndex()]);
            LOG_F(INFO, "Loaded texture: baseColorTexture");
        }
        if (gmat.normalTexture.index > -1)
        {
            material.textures.push_back(textures[gmat.normalTexture.index]);
            LOG_F(INFO, "Loaded texture: normalTexture");
        }
        if (gmat.pbrMetallicRoughness.metallicRoughnessTexture.index > -1)
        {
            material.textures.push_back(textures[gmat.pbrMetallicRoughness.metallicRoughnessTexture.index]);
            LOG_F(INFO, "Loaded texture: pbrMetallicRoughness");
        }
        if (gmat.occlusionTexture.index > -1)
        {
            material.textures.push_back(textures[gmat.occlusionTexture.index]);
            LOG_F(INFO, "Loaded texture: occlusionTexture");
        }
        if (gmat.emissiveTexture.index > -1)
        {
            material.textures.push_back(textures[gmat.emissiveTexture.index]);
            LOG_F(INFO, "Loaded texture: emissiveTexture");
        }
        if (gmat.values.count("baseColorFactor"))
        {
            material.diffuseColor = glm::make_vec4(gmat.values["baseColorFactor"].ColorFactor().data());
        }
        ret.push_back(r->createEntity(material));
    }
    return ret;
}

std::vector<Handle> importNodes(Registry *r, tinygltf::Model &model, std::map<int, Handle> &meshes)
{
    std::vector<Handle> nodes;
    for (auto gnode : model.nodes)
    {
        auto node = r->nextEntityId();
        Transform transform(gnode.translation.size() ? glm::make_vec3(gnode.translation.data()) : glm::highp_dvec3(),
                                    gnode.rotation.size() ? glm::make_quat(gnode.rotation.data()) : glm::highp_dquat(),
                                    gnode.scale.size() ? glm::make_vec3(gnode.scale.data()) : glm::highp_dvec3(1.0));
        if (gnode.matrix.size() == 16)
            transform.matrix(mat4(glm::make_mat4x4(gnode.matrix.data())));
        
        r->addComponent(node, transform);
        r->addComponent(node, Relation());
        r->addComponent(node, Info{gnode.name, false});
        if (gnode.mesh > -1)
        {
            r->addComponent(node, Renderable{meshes[gnode.mesh]});
            BBox bbox;
            for (auto geoHandle : r->get<Mesh>(meshes[gnode.mesh]).geometries)
            {
                auto &geo = r->get<Geometry>(geoHandle);
                bbox += geo.bbox;
            }
            r->addComponent(node, bbox);
        }
        nodes.push_back(node);
    }

    for (auto i = 0; i < nodes.size(); i++)
    {
        for (auto ci : model.nodes[i].children)
        {
            auto& [rel] = r->getEntity<Relation>(nodes[i]);
            rel.add(nodes[ci]);
            auto& [crel] = r->getEntity<Relation>(nodes[ci]);
            crel.parent = nodes[i];
        }
    }
    return nodes;
}