#pragma once

#include <AccelerationStructures.hpp>
#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <ObjectMouseSelect.hpp>
#include <Particle.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

#define VK_GLTF_MATERIAL_IDS

class MainRenderer {
 private:
  // acceleration structures
  gtp::AccelerationStructures accelerationStructures;

  // tlas particle update refactor later
  //bool updateTLAS = false;

  // -- buffers
  struct Buffers {
    gtp::Buffer ubo{};
  };

  // -- shader binding table data
  Utilities_Renderer::ShaderBindingTableData shaderBindingTableData{};

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- storage image
  Utilities_AS::StorageImage storageImage{};

  // -- uniform data
  Utilities_Renderer::UniformData uniformData{};

  // -- Buffers
  Buffers buffers;

  // -- pipeline data
  Utilities_Renderer::PipelineData pipelineData{};

  // -- shader
  gtp::Shader shader;

  // -- compute
  std::vector<ComputeVertex *> gltfCompute;

  // -- init class function
  void Init_MainRenderer(EngineCore *pEngineCore);

  // -- destroy class objects
  void Destroy_MainRenderer();

  // -- assets data struct
  struct Assets {
    std::vector<gtp::TextureLoader> defaultTextures;

    // models
    std::vector<gtp::Model *> models;
    Utilities_UI::ModelData modelData;
    Utilities_UI::LoadModelFlags loadModelFlags;

    // colored glass texture
    gtp::TextureLoader coloredGlassTexture;

    // cube map
    gtp::TextureLoader cubemap;

    // particle
    std::vector<gtp::Particle *> particle;
  };

  // -- assets
  Assets assets{};

  // -- load model
  void LoadModel(std::string filename, uint32_t fileLoadingFlags = 0,
                 Utilities_Renderer::ModelLoadingFlags modelLoadingFlags =
                     Utilities_Renderer::ModelLoadingFlags::None,
                 Utilities_UI::TransformMatrices *pTransformMatrices = nullptr);

  // -- load particle
  void LoadParticle(std::string filename, uint32_t fileLoadingFlags,
                    Utilities_Renderer::ModelLoadingFlags modelLoadingFlags,
                    Utilities_UI::TransformMatrices *pTransformMatrices);

  // -- Setup Model Transforms
  void SetupModelDataTransforms(
      Utilities_UI::TransformMatrices *pTransformMatrices);

  // -- load assets
  void LoadAssets();

  // -- load gltf compute
  void LoadGltfCompute(gtp::Model *pModel);

  // -- create storage image
  void CreateStorageImages();

  // -- create uniform buffer
  void CreateUniformBuffer();

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

  // -- create geometry nodes buffer
  // void CreateGeometryNodesBuffer();

  /* tools */
  struct Tools {
    gtp::ObjectMouseSelect objectMouseSelect;
  };

  Tools tools{};

 public:
  // -- constructor
  MainRenderer();

  // -- init constructor
  MainRenderer(EngineCore *pEngineCore);

  // -- get main renderer compute instances
  std::vector<ComputeVertex *> GetComputeInstances();

  // -- update uniform buffer
  void UpdateUniformBuffer(float deltaTime, glm::vec4 lightPosition);

  // -- update ray tracing pipeline
  void UpdateRayTracingPipeline();

  // -- rebuild command buffers
  void RebuildCommandBuffers(int frame, bool showObjectColorID);

  // -- handle acceleration structure update
  void HandleAccelerationStructureUpdate();

  // -- delete model
  void DeleteModel();

  // -- update descriptor set
  void UpdateDescriptorSet();

  // -- update animations
  void UpdateAnimations(float deltaTime);

  // -- record particle compute commands
  VkCommandBuffer RecordParticleComputeCommands(
    int currentFrame, int particleIndex);

  // -- handle window resize
  void HandleResize();

  // -- handle load model
  void HandleLoadModel(gtp::FileLoadingFlags loadingFlags);

  // -- get asset model data pointer from renderer
  Utilities_UI::ModelData *GetModelData();

  // -- set renderer asset model data
  void SetModelData(Utilities_UI::ModelData *pModelData);

  // -- get tools
  MainRenderer::Tools GetTools();

  std::vector<VkCommandBuffer> RecordCompute(int frame);

  // -- Destroy
  void Destroy();
};
