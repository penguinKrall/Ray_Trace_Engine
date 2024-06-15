#pragma once

#include <Shader.hpp>
#include <Utilities_EngCore.hpp>
#include <Utilities_UI.hpp>
#include <filesystem>
#include <glTFModel.hpp>

class ComputeVertex {

public:

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- shader
  gtp::Shader shader;

  // -- gltf_model
  // this is where the vertex buffers are that will be used by the compute
  // shader and subsequently the ray tracing shaders
  gtp::Model *model = nullptr;

  // -- shader storage buffer
  gtp::Buffer storageInputBuffer;
  gtp::Buffer storageOutputBuffer;

  // -- joint storage buffer
  gtp::Buffer jointBuffer;

  // -- rotate/translate/scale matrices buffer
  Utilities_UI::TransformMatrices transformMatrices{};
  //Utilities_UI::ModelData uiModelData{};

  gtp::Buffer transformsBuffer;

  // -- pipeline data struct
  struct PipelineData {
    VkDescriptorPool descriptorPool{};
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSet descriptorSet{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
  };

  // -- command buffers
  std::vector<VkCommandBuffer> commandBuffers;

  // -- pipeline data
  PipelineData pipelineData{};

  // -- default constructor
  ComputeVertex();

  // -- init constructor
  ComputeVertex(EngineCore *coreBase, gtp::Model *modelPtr);

  // -- init func
  void Init_ComputeVertex(EngineCore *coreBase, gtp::Model *modelPtr);

  // -- create transforms buffer
  void CreateTransformsBuffer();

  // -- update transforms buffer
  void UpdateTransformsBuffer(Utilities_UI::TransformMatrices* pTransformMatricesData);

  // -- initialize model data
  void SetModelData(Utilities_UI::ModelData* pModelData, int modelIndex) {
    //this->modelData.transformMatrices.rotate = glm::rotate(
    //  glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    //this->modelData.transformMatrices.translate =
    //  glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.0f, 0.0f));

    //this->modelData.transformMatrices.scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));
    
    //initialize model data values
   // this->modelData.transformMatrices = pModelData->transformMatrices;

    this->UpdateTransformsBuffer(&pModelData->transformMatrices[modelIndex]);

  }

  // -- create buffers
  void CreateComputeBuffers();

  // -- create pipeline
  void CreateComputePipeline();

  // -- create descriptor sets
  void CreateDescriptorSets();

  // -- create command buffers
  void CreateCommandBuffers();

  // -- update joint buffers
  void UpdateJointBuffer();

  // -- record compute commands
  void RecordComputeCommands(int frame);

  // -- retrieve buffer data
  // std::vector<gtp::Model::Vertex> RetrieveBufferData();

  // -- destroy
  void Destroy_ComputeVertex();
};
