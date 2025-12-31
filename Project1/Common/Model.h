#pragma once
#include "vulkan/vulkan.h"
#include <vector>
#include <array>
#include <stdexcept>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include "../Libraries/tinyobjloader-release/tiny_obj_loader.h"
#include <unordered_map>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(6);
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, normal);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, tangent);

        attributeDescriptions[5].binding = 0;
        attributeDescriptions[5].location = 5;
        attributeDescriptions[5].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[5].offset = offsetof(Vertex, bitangent);
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1) ^ (hash<glm::vec3>()(vertex.normal) << 1);
        }
    };
}

struct Model
{
    std::vector<uint32_t> indices;
    std::vector<Vertex> vertices;

    void loadModel(const std::string& path) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};
        for (const auto& shape : shapes) {
            for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
                const tinyobj::index_t& index0 = shape.mesh.indices[i];
                const tinyobj::index_t& index1 = shape.mesh.indices[i + 1];
                const tinyobj::index_t& index2 = shape.mesh.indices[i + 2];
                glm::vec3 v0 = {
                    attrib.vertices[3 * index0.vertex_index + 0],
                    attrib.vertices[3 * index0.vertex_index + 1],
                    attrib.vertices[3 * index0.vertex_index + 2]
                };
                glm::vec3 v1 = {
                    attrib.vertices[3 * index1.vertex_index + 0],
                    attrib.vertices[3 * index1.vertex_index + 1],
                    attrib.vertices[3 * index1.vertex_index + 2]
                };
                glm::vec3 v2 = {
                    attrib.vertices[3 * index2.vertex_index + 0],
                    attrib.vertices[3 * index2.vertex_index + 1],
                    attrib.vertices[3 * index2.vertex_index + 2]
                };
                glm::vec2 t0 = {
                    attrib.texcoords[2 * index0.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index0.texcoord_index + 1]
                };
                glm::vec2 t1 = {
                    attrib.texcoords[2 * index1.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index1.texcoord_index + 1]
                };
                glm::vec2 t2 = {
                    attrib.texcoords[2 * index2.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index2.texcoord_index + 1]
                };
                glm::vec3 n0 = {
                    attrib.normals[3 * index0.normal_index + 0],
                    attrib.normals[3 * index0.normal_index + 1],
                    attrib.normals[3 * index0.normal_index + 2],
                };
                glm::vec3 n1 = {
                    attrib.normals[3 * index1.normal_index + 0],
                    attrib.normals[3 * index1.normal_index + 1],
                    attrib.normals[3 * index1.normal_index + 2],
                };
                glm::vec3 n2 = {
                    attrib.normals[3 * index2.normal_index + 0],
                    attrib.normals[3 * index2.normal_index + 1],
                    attrib.normals[3 * index2.normal_index + 2],
                };

                glm::vec3 edge1 = v1 - v0;
                glm::vec3 edge2 = v2 - v0;
                glm::vec2 deltaUV1 = t1 - t0;
                glm::vec2 deltaUV2 = t2 - t0;
                float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
                glm::vec3 tangent = {
                    f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x),
                    f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y),
                    f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z)
                };
                glm::vec3 T = tangent + tangent + tangent;
                
                glm::vec3 N0 = glm::normalize(n0);
                glm::vec3 T0 = glm::normalize(T - N0 * glm::dot(N0,T));
                glm::vec3 B0 = glm::normalize(glm::cross(N0, T0));
                Vertex vertex0 {};
                vertex0.pos = v0;
                vertex0.texCoord = t0;
                vertex0.normal = n0;
                vertex0.color = {1.0f, 1.0f, 1.0f};
                vertex0.tangent = T0;
                vertex0.bitangent = B0;
                if (uniqueVertices.count(vertex0) == 0) {
                    uniqueVertices[vertex0] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex0);
                }
                indices.push_back(uniqueVertices[vertex0]);

                glm::vec3 N1 = glm::normalize(n1);
                glm::vec3 T1 = glm::normalize(T - N1 * glm::dot(N1,T));
                glm::vec3 B1 = glm::normalize(glm::cross(N1, T1));
                Vertex vertex1 {};
                vertex1.pos = v1;
                vertex1.texCoord = t1;
                vertex1.normal = n1;
                vertex1.color = {1.0f, 1.0f, 1.0f};
                vertex1.tangent = T1;
                vertex1.bitangent = B1;
                if (uniqueVertices.count(vertex1) == 0) {
                    uniqueVertices[vertex1] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex1);
                }
                indices.push_back(uniqueVertices[vertex1]);

                glm::vec3 N2 = glm::normalize(n2);
                glm::vec3 T2 = glm::normalize(T - N2 * glm::dot(N2,T));
                glm::vec3 B2 = glm::normalize(glm::cross(N2, T2));
                Vertex vertex2 {};
                vertex2.pos = v2;
                vertex2.texCoord = t2;
                vertex2.normal = n2;
                vertex2.color = {1.0f, 1.0f, 1.0f};
                vertex2.tangent = T2;
                vertex2.bitangent = B2;
                if (uniqueVertices.count(vertex2) == 0) {
                    uniqueVertices[vertex2] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex2);
                }
                indices.push_back(uniqueVertices[vertex2]);
            }
        }
    }
    
    void clearModel() {
        vertices.clear();
        indices.clear();
    }
};

