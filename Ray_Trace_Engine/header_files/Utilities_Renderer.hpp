#pragma once

#include <EngineCore.hpp>
#include <glTFModel.hpp>

class Utilities_Renderer {
public:
  struct TransformsData {
    glm::mat4 rotate = glm::mat4(1.0f);
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
    gtp::Model *model = nullptr;
  };

  static std::vector<gtp::Model::Vertex>
  GetVerticesFromBuffer(VkDevice device, gtp::Model *model) {

    void *data = nullptr;

    VkDeviceSize bufferSize =
        static_cast<uint32_t>(model->vertexCount) * sizeof(gtp::Model::Vertex);

    if (vkMapMemory(device, model->vertices.memory, 0, bufferSize, 0, &data) !=
        VK_SUCCESS) {
      std::cerr
          << "\nUtilities_AS::GetVerticesFromBuffer failed to map memory\n";
    }

    std::vector<gtp::Model::Vertex> tempVertexBuffer{};
    tempVertexBuffer.resize(static_cast<uint32_t>(model->vertexCount));

    memcpy(tempVertexBuffer.data(), data, bufferSize);

    vkUnmapMemory(device, model->vertices.memory);

    return tempVertexBuffer;
  }

  static void TransformModel(EngineCore *engineCore,
                             TransformsData *transformsData) {

    std::vector<gtp::Model::Vertex> tempSceneVerticesBuffer;

    tempSceneVerticesBuffer = GetVerticesFromBuffer(engineCore->devices.logical,
                                                    transformsData->model);

    VkDeviceSize tempSceneBufferSize =
        static_cast<uint32_t>(tempSceneVerticesBuffer.size()) *
        sizeof(gtp::Model::Vertex);

    std::cout << "tempSceneVerticesBuffer.size(): "
              << tempSceneVerticesBuffer.size() << std::endl;

    for (int i = 0; i < tempSceneVerticesBuffer.size(); i++) {
      tempSceneVerticesBuffer[i].pos =
          transformsData->rotate * tempSceneVerticesBuffer[i].pos;
      tempSceneVerticesBuffer[i].pos =
          transformsData->translate * tempSceneVerticesBuffer[i].pos;
      tempSceneVerticesBuffer[i].pos =
          transformsData->scale * tempSceneVerticesBuffer[i].pos;
    }

    void *verticesData;

    vkMapMemory(engineCore->devices.logical,
                transformsData->model->vertices.memory, 0, tempSceneBufferSize,
                0, &verticesData);

    memcpy(verticesData, tempSceneVerticesBuffer.data(), tempSceneBufferSize);

    vkUnmapMemory(engineCore->devices.logical,
                  transformsData->model->vertices.memory);
  }
};
