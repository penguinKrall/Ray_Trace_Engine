#pragma once

#include <EngineCore.hpp>
#include <glTFModel.hpp>

class Utilities_Renderer {
public:

  enum ModelLoadingFlags {
    None = 0x00000000,
    Animated = 0x00000001,
    SemiTransparent = 0x00000002,
    PositionModel = 0x00000003
  };

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

  static void TransformModelVertices(EngineCore *engineCore,
                                     TransformsData *transformsData) {

    // std::vector<gtp::Model::Vertex> tempSceneVerticesBuffer;

    //transformsData->model->verticesBuffer = GetVerticesFromBuffer(engineCore->devices.logical,
    //                                                transformsData->model);

    //VkDeviceSize tempSceneBufferSize =
    //    static_cast<uint32_t>(tempSceneVerticesBuffer.size()) *
    //    sizeof(gtp::Model::Vertex);

    std::cout << "transformsData->model->verticesBuffer.size(): "
              << transformsData->model->verticesBuffer.size() << std::endl;

    for (int i = 0; i < transformsData->model->verticesBuffer.size(); i++) {
      transformsData->model->verticesBuffer[i].pos =
        transformsData->rotate * transformsData->model->verticesBuffer[i].pos;
      transformsData->model->verticesBuffer[i].pos =
        transformsData->translate * transformsData->model->verticesBuffer[i].pos;
      transformsData->model->verticesBuffer[i].pos =
        transformsData->scale * transformsData->model->verticesBuffer[i].pos;
    }

    void* verticesData;

    vkMapMemory(engineCore->devices.logical,
      transformsData->model->vertices.memory, 0, transformsData->model->vertexBufferSize,
      0, &verticesData);

    memcpy(verticesData, transformsData->model->verticesBuffer.data(), transformsData->model->vertexBufferSize);

    vkUnmapMemory(engineCore->devices.logical,
                  transformsData->model->vertices.memory);
  }

  /*static void TransformModelVertices2(EngineCore* engineCore,
    TransformsData* transformsData) {

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

    void* verticesData;

    vkMapMemory(engineCore->devices.logical,
      transformsData->model->vertices.memory, 0, tempSceneBufferSize,
      0, &verticesData);

    memcpy(verticesData, tempSceneVerticesBuffer.data(), tempSceneBufferSize);

    vkUnmapMemory(engineCore->devices.logical,
      transformsData->model->vertices.memory);
  }*/

};
