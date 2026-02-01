#include "gl_loadGLTF.hpp"
#include <iostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

GlModel::GlModel(const char* filename)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) std::cout << "Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "Error: " << err << std::endl;
    if (!res) std::cerr << "Failed to load glTF: " << filename << std::endl;

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    for (int nodeIndex : scene.nodes)
    {
        processNode(model, model.nodes[nodeIndex], glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)));
    }

    if (triangles.empty()) return;

    float logicalWidth  = 570.0f;
    float logicalHeight = 570.0f;
    float logicalDepth  = 570.0f;

    glm::vec3 center = (globalMin + globalMax) * 0.5f;
    glm::vec3 size   = globalMax - globalMin;

    float scaleX = logicalWidth  / size.x;
    float scaleY = logicalHeight / size.y;
    float scaleZ = logicalDepth  / size.z;
    float finalScale = std::min({scaleX, scaleY, scaleZ}) * 0.95f;

    glm::vec3 gridCenter(logicalWidth / 2.0f, logicalHeight / 2.0f, logicalDepth / 2.0f);

    for (auto& tri : triangles)
    {
        tri.v0 = glm::vec4(((glm::vec3(tri.v0) - center) * finalScale) + gridCenter, 1.0f);
        tri.v1 = glm::vec4(((glm::vec3(tri.v1) - center) * finalScale) + gridCenter, 1.0f);
        tri.v2 = glm::vec4(((glm::vec3(tri.v2) - center) * finalScale) + gridCenter, 1.0f);
    }
}
void GlModel::processNode(tinygltf::Model& model, const tinygltf::Node& node, glm::mat4 parentTransform)
{
    glm::mat4 localTransform = glm::mat4(1.0f);
    if (node.matrix.size() == 16)
    {
        localTransform = glm::make_mat4(node.matrix.data());
    }
    else
    {
        if (node.translation.size() == 3) 
            localTransform = glm::translate(localTransform, glm::vec3(node.translation[0], node.translation[1], node.translation[2]));
        if (node.rotation.size() == 4) 
            localTransform *= glm::mat4_cast(glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]));
        if (node.scale.size() == 3) 
            localTransform = glm::scale(localTransform, glm::vec3(node.scale[0], node.scale[1], node.scale[2]));
    }

    glm::mat4 worldTransform = parentTransform * localTransform;

    if (node.mesh >= 0)
    {
        const tinygltf::Mesh& mesh = model.meshes[node.mesh];
        for (const auto& primitive : mesh.primitives)
        {
            extractPrimitive(model, primitive, worldTransform);
        }
    }
    for (int childIndex : node.children)
    {
        processNode(model, model.nodes[childIndex], worldTransform);
    }
}
void GlModel::extractPrimitive(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const glm::mat4& transform) {
    const auto& posAttrib = primitive.attributes.find("POSITION");
    if (posAttrib == primitive.attributes.end()) return;

    const auto& posAccessor = model.accessors[posAttrib->second];
    const auto& posView = model.bufferViews[posAccessor.bufferView];
    const auto& posBuffer = model.buffers[posView.buffer];
    const float* posPtr = reinterpret_cast<const float*>(&(posBuffer.data[posView.byteOffset + posAccessor.byteOffset]));

    const auto& idxAccessor = model.accessors[primitive.indices];
    const auto& idxView = model.bufferViews[idxAccessor.bufferView];
    const auto& idxBuffer = model.buffers[idxView.buffer];
    const void* idxPtr = &(idxBuffer.data[idxView.byteOffset + idxAccessor.byteOffset]);

    for (size_t i = 0; i < idxAccessor.count; i += 3) {
        uint32_t i0, i1, i2;

        if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* p = static_cast<const uint32_t*>(idxPtr);
            i0 = p[i]; i1 = p[i+1]; i2 = p[i+2];
        } else if (idxAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* p = static_cast<const uint16_t*>(idxPtr);
            i0 = p[i]; i1 = p[i+1]; i2 = p[i+2];
        } else {
            const uint8_t* p = static_cast<const uint8_t*>(idxPtr);
            i0 = p[i]; i1 = p[i+1]; i2 = p[i+2];
        }

        GpuTriangle tri;
        tri.v0 = transform * glm::vec4(posPtr[i0 * 3 + 0], posPtr[i0 * 3 + 1], posPtr[i0 * 3 + 2], 1.0f);
        tri.v1 = transform * glm::vec4(posPtr[i1 * 3 + 0], posPtr[i1 * 3 + 1], posPtr[i1 * 3 + 2], 1.0f);
        tri.v2 = transform * glm::vec4(posPtr[i2 * 3 + 0], posPtr[i2 * 3 + 1], posPtr[i2 * 3 + 2], 1.0f);

        globalMin = glm::min(globalMin, glm::vec3(tri.v0));
        globalMin = glm::min(globalMin, glm::vec3(tri.v1));
        globalMin = glm::min(globalMin, glm::vec3(tri.v2));

        globalMax = glm::max(globalMax, glm::vec3(tri.v0));
        globalMax = glm::max(globalMax, glm::vec3(tri.v1));
        globalMax = glm::max(globalMax, glm::vec3(tri.v2));

        triangles.push_back(tri);
    }
}