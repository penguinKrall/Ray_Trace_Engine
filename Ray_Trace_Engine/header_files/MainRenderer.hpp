#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

#define VK_GLTF_MATERIAL_IDS

class MainRenderer {
public:
  std::vector<Utilities_AS::GeometryNode> geometryNodeBuf;
  std::vector<Utilities_AS::GeometryIndex> geometryIndexBuf;

  // -- uniform data struct
  struct UniformData {
    glm::mat4 viewInverse = glm::mat4(1.0f);
    glm::mat4 projInverse = glm::mat4(1.0f);
    glm::vec4 lightPos = glm::vec4(0.0f);
  };

  // storage buffer data struct
  struct StorageBufferData {
    std::vector<int> reflectGeometryID;
  };

  // -- buffers
  struct Buffers {
    gtp::Buffer transformBuffer{};
    gtp::Buffer g_nodes_buffer{};
    gtp::Buffer g_nodes_indices{};
    gtp::Buffer blas_scratch{};
    gtp::Buffer second_blas_scratch{};
    gtp::Buffer tlas_scratch{};
    gtp::Buffer tlas_instancesBuffer;
    gtp::Buffer ubo;
  };

  // -- assets data struct
  struct Assets {

    // models
    std::vector<gtp::Model *> models;
    Utilities_UI::ModelData modelData;

    // animation model
    // gtp::Model *animatedModel;

    // static/scene model
    // gtp::Model *helmetModel;

    ////static/scene model
    // gtp::Model *testScene;

    ////building glass model
    // gtp::Model *waterSurface;

    // colored glass texture
    gtp::TextureLoader coloredGlassTexture;
  };

  // -- pipeline data struct
  struct PipelineData {
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
    VkDescriptorSet descriptorSet{};
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorPool descriptorPool{};
  };

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

  // -- top level acceleration structure
  Utilities_AS::AccelerationStructure TLAS{};

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- storage image
  Utilities_AS::StorageImage storageImage{};

  // -- uniform data
  UniformData uniformData{};

  // -- storage buffer data
  StorageBufferData storageBufferData{};

  // -- bottom level acceleration structures
  std::vector<Utilities_AS::BLASData> bottomLevelAccelerationStructures;

  // -- TLAS data
  Utilities_AS::TLASData tlasData{};

  // -- Buffers
  Buffers buffers;

  // -- assets
  Assets assets{};

  // -- pipeline data
  PipelineData pipelineData{};

  // -- ui data
  // Utilities_UI::ModelData uiModelData{};

  // -- shader
  gtp::Shader shader;

  // -- compute
  ComputeVertex gltfCompute;

  // -- constructor
  MainRenderer();

  // -- init constructor
  MainRenderer(EngineCore *coreBase);

  // -- init class function
  void Init_MainRenderer(EngineCore *coreBase);

  // -- load model
  void LoadModel(std::string filename, uint32_t fileLoadingFlags = 0,
                 Utilities_Renderer::ModelLoadingFlags modelLoadingFlags =
                     Utilities_Renderer::ModelLoadingFlags::None,
                 Utilities_UI::TransformMatrices *pTransformMatrices = nullptr);

  // -- load assets
  void LoadAssets();

  // -- create bottom level acceleration structures
  void CreateBottomLevelAccelerationStructures();

  // -- create top level acceleration structure
  void CreateTLAS();

  // -- create storage image
  void CreateStorageImage();

  // -- create uniform buffer
  void CreateUniformBuffer();

  // -- update uniform buffer
  void UpdateUniformBuffer(float deltaTime);

  // -- create ray tracing pipeline
  void CreateRayTracingPipeline();

  // -- create shader binding tables
  void CreateShaderBindingTable();

  // -- create descriptor set
  void CreateDescriptorSet();

  // -- setup buffer region addresses
  void SetupBufferRegionAddresses();

  // -- build command buffers
  void BuildCommandBuffers();

  // -- rebuild command buffers
  void RebuildCommandBuffers(int frame);

  // -- update BLAS
  void UpdateBLAS();

  // -- update TLAS
  void UpdateTLAS();

  // -- pre transform animation model vertices
  void PreTransformModels();

  // -- create geometry nodes buffer
  void CreateGeometryNodesBuffer();

  // -- update descriptor set
  void UpdateDescriptorSet();

  // -- handle window resize
  void HandleResize();

  // -- update UI data
  void UpdateUIData(Utilities_UI::ModelData *pModelData);


  // -- update model transforms
  void UpdateModelTransforms(Utilities_UI::ModelData* pModelData);

  // -- destroy class objects
  void Destroy_MainRenderer();
};
