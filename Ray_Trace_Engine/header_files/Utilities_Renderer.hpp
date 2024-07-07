#pragma once

#include <EngineCore.hpp>
#include <glTFModel.hpp>

class Utilities_Renderer {
public:

  // -- pipeline data struct
  struct PipelineData {
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
    VkDescriptorSet descriptorSet{};
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorPool descriptorPool{};
  };

  // -- shader binding table data
  struct ShaderBindingTableData {
    // -- raytracing ray generation shader binding table
    gtp::Buffer raygenShaderBindingTable;
    VkStridedDeviceAddressRegionKHR raygenStridedDeviceAddressRegion{};

    // -- raytracing miss shader binding table
    gtp::Buffer missShaderBindingTable;
    VkStridedDeviceAddressRegionKHR missStridedDeviceAddressRegion{};

    // -- raytracing hit shader binding table
    gtp::Buffer hitShaderBindingTable;
    VkStridedDeviceAddressRegionKHR hitStridedDeviceAddressRegion{};

    // -- shader groups
    std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};
  };

  // -- uniform data struct
  struct UniformData {
    glm::mat4 viewInverse = glm::mat4(1.0f);
    glm::mat4 projInverse = glm::mat4(1.0f);
    glm::vec4 lightPos = glm::vec4(0.0f);
    glm::vec4 viewPos = glm::vec4(0.0f);
  };

  // -- model loading flags
  enum ModelLoadingFlags {
    None = 0x00000000,
    Animated = 0x00000001,
    SemiTransparent = 0x00000002,
    PositionModel = 0x00000003
  };

  // -- transforms data
  //@brief -- transform matrices and model ptr
  struct TransformsData {
    glm::mat4 rotate = glm::mat4(1.0f);
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
    gtp::Model *model = nullptr;
  };

  // -- get vertices from buffer
  //@brief -- returns a vector of gt::Model::Vertex from a VkBuffer of vertices
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

};
