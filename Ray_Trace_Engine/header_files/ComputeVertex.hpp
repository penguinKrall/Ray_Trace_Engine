#pragma once

#include <Shader.hpp>
#include <Utilities_EngCore.hpp>
#include <Utilities_UI.hpp>
#include <filesystem>
#include <glTFModel.hpp>

class ComputeVertex {
private:
  struct Geometry {
    double textureIndexBaseColor = -1;
    double textureIndexOcclusion = -1;
    double textureIndexMetallicRoughness = -1;
    double textureIndexNormal = -1;
    double firstVertex = 0;
    double vertexCount = 0;
  };

  struct GeometryBufferData {
  std::vector<Geometry> geometries;
  };

  GeometryBufferData geometryBufferData{};

  // struct GeometryBufferData {
  //   std::vector<GeometryData> geometryData;
  // };
  // GeometryBufferData geometryBufferData{};

  // struct GeometryIndexData {
  //   int geometryCount = 0;
  // };
  // GeometryIndexData geometryIndexData{};

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- shader
  gtp::Shader shader;

  // -- gltf_model
  // this is where the vertex buffers are that will be used by the compute
  // shader and subsequently the ray tracing shaders
  gtp::Model *model = nullptr;

  // -- shader storage buffer
  // gtp::Buffer storageInputBuffer;
  gtp::Buffer vertexReadBuffer;

  // -- bone storage buffer
  gtp::Buffer boneBuffer;

  // -- geometry buffer
  gtp::Buffer geometryBuffer;

  gtp::Buffer transformMatrixBuffer;

  // -- rotate/translate/scale matrices buffer
  Utilities_UI::TransformMatrices transformMatrices{};

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
  void CreateGeometryBuffer();

public:
  // -- default constructor
  ComputeVertex();

  // -- init constructor
  ComputeVertex(EngineCore *pEngineCore, gtp::Model *modelPtr);

  // -- init func
  void Init_ComputeVertex(EngineCore *pEngineCore, gtp::Model *modelPtr);

  // -- create transforms buffer
  void CreateTransformMatrixBuffer();

  // -- create buffers
  void CreateBuffers();

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

  // -- update bone buffers
  void UpdateBoneBuffer();

  // -- update transforms buffer
  void
  UpdateTransformMatrixBuffer(Utilities_UI::TransformMatrices *pTransformMatrices);

  // -- get bone buffer
  gtp::Buffer *GetBoneBuffer();

  // -- record compute commands
  VkCommandBuffer RecordComputeCommands(int frame);

  // -- destroy
  void Destroy_ComputeVertex();
};
