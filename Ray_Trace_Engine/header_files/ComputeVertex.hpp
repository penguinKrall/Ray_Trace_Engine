#pragma once

#include <Shader.hpp>
#include <Utilities_EngCore.hpp>
#include <Utilities_UI.hpp>
#include <filesystem>
#include <glTFModel.hpp>

class ComputeVertex {
private:
  struct GeometryData {
    int textureIndexBaseColor = -1;
    int textureIndexOcclusion = -1;
    int textureIndexMetallicRoughness = -1;
    int textureIndexNormal = -1;
    int firstVertex = 0;
    int vertexCount = 0;
  };

  std::vector<GeometryData> geometryData;

  //struct GeometryBufferData {
  //  std::vector<GeometryData> geometryData;
  //};
  //GeometryBufferData geometryBufferData{};

  struct GeometryIndexData {
    int geometryCount = 0;
  };
  GeometryIndexData geometryIndexData{};

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

  // -- uniform buffer
  gtp::Buffer uniformBuffer;

  // -- rotate/translate/scale matrices buffer
  Utilities_UI::TransformMatrices transformMatrices{};
  // Utilities_UI::ModelData uiModelData{};

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

  // -- create uniform buffer
  void CreateUniformBuffer();

public:
  // -- default constructor
  ComputeVertex();

  // -- init constructor
  ComputeVertex(EngineCore *pEngineCore, gtp::Model *modelPtr);

  // -- init func
  void Init_ComputeVertex(EngineCore *pEngineCore, gtp::Model *modelPtr);

  // -- create transforms buffer
  void CreateTransformsBuffer();

  // -- create buffers
  void CreateComputeBuffers();

  // -- create animation pipeline
  void CreateAnimationComputePipeline();

  // -- create animation pipeline
  void CreateStaticComputePipeline();

  // -- create animation pipeline descriptor set
  void CreateAnimationPipelineDescriptorSet();

  // -- create static pipeline descriptor set
  void CreateStaticPipelineDescriptorSet();

  // -- create command buffers
  void CreateCommandBuffers();

  // -- update joint buffers
  void UpdateJointBuffer();

  // -- update transforms buffer
  void
  UpdateTransformsBuffer(Utilities_UI::TransformMatrices *pTransformMatrices);

  // -- get joint buffer
  gtp::Buffer *GetJointBuffer();

  // -- record compute commands
  VkCommandBuffer RecordComputeCommands(int frame);

  // -- destroy
  void Destroy_ComputeVertex();
};
