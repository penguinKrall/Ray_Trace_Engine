#pragma once

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
#include <AccelerationStructures.hpp>

#define VK_GLTF_MATERIAL_IDS

class MainRenderer {
 private:

   //acceleration structures
   gtp::AccelerationStructures accelerationStructures;

  // tlas particle update refactor later
  bool updateTLAS = false;

  std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

  // -- geometry node vector
  std::vector<Utilities_AS::GeometryNode> geometryNodes;

  // -- geometry node index vector
  std::vector<Utilities_AS::GeometryIndex> geometryNodesIndices;

  // -- buffers
  struct Buffers {
    gtp::Buffer transformBuffer{};
    gtp::Buffer geometry_nodes_buffer{};
    gtp::Buffer geometry_nodes_indices{};
    gtp::Buffer blas_scratch{};
    gtp::Buffer tlas_scratch{};
    gtp::Buffer tlas_instancesBuffer{};
    gtp::Buffer ubo{};
  };

  // -- shader binding table data
  Utilities_Renderer::ShaderBindingTableData shaderBindingTableData{};

  // -- top level acceleration structure
  Utilities_AS::AccelerationStructure TLAS{};

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- storage image
  Utilities_AS::StorageImage storageImage{};

  // -- uniform data
  Utilities_Renderer::UniformData uniformData{};

  // -- bottom level acceleration structures
  std::vector<Utilities_AS::BLASData *> bottomLevelAccelerationStructures;

  // -- TLAS data
  Utilities_AS::TLASData tlasData{};

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
    uint32_t textureOffset = 0;
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

  // -- create BLAS
  void CreateBLAS(gtp::Model *pModel);

  // -- create top level acceleration structure
  void CreateTLAS();

  // -- load gltf compute
  void LoadGltfCompute(gtp::Model *pModel);

  // -- handle particle tlas instances
  void InitializeParticleBLASInstances(int particleIdx);

  // -- create storage image
  void CreateStorageImages();

  // -- create color id image buffer
  // void CreateColorIDImageBuffer();

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
  void CreateGeometryNodesBuffer();

 public:
  /* tools */
  struct Tools {
    gtp::ObjectMouseSelect objectMouseSelect;
  };

  Tools tools{};

  // -- constructor
  MainRenderer();

  // -- init constructor
  MainRenderer(EngineCore *pEngineCore);

  // -- get main renderer compute instances
  std::vector<ComputeVertex *> GetComputeInstances();

  // -- retrieve object id from image
  // -- copy object id image to buffer
  // void RetrieveObjectIDFromImage();

  // -- update uniform buffer
  void UpdateUniformBuffer(float deltaTime, glm::vec4 lightPosition);

  // -- update ray tracing pipeline
  void UpdateRayTracingPipeline();

  // -- rebuild command buffers
  void RebuildCommandBuffers(int frame, bool showObjectColorID);

  // -- update BLAS
  void UpdateBLAS();

  // -- update TLAS
  void UpdateTLAS();

  //// -- create geometry nodes buffer
  // void CreateGeometryNodesBuffer();

  // -- update geometry nodes buffer
  void UpdateGeometryNodesBuffer(gtp::Model *pModel);

  // -- delete model
  void DeleteModel();

  // -- update descriptor set
  void UpdateDescriptorSet();

  // -- update animations
  void UpdateAnimations(float deltaTime);

  // -- record particle compute commands
  std::vector<VkCommandBuffer> RecordParticleComputeCommands(
      int currentFrame, std::vector<VkCommandBuffer> computeCommandBuffer);

  // -- handle window resize
  void HandleResize();

  // -- handle load model
  void HandleLoadModel(gtp::FileLoadingFlags loadingFlags);

  // -- get asset model data pointer from renderer
  Utilities_UI::ModelData *GetModelData();

  // -- set renderer asset model data
  void SetModelData(Utilities_UI::ModelData *pModelData);

  // -- Destroy
  void Destroy();
};
