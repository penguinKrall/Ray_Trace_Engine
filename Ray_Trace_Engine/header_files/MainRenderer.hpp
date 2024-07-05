#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <Particle.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

#define VK_GLTF_MATERIAL_IDS

class MainRenderer {
public:

  //tlas particle update refactor later
  bool updateTLAS = false;
  std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

  // -- geometry node vector
  std::vector<Utilities_AS::GeometryNode> geometryNodeBuf;

  // -- geometry node index vector
  std::vector<Utilities_AS::GeometryIndex> geometryIndexBuf;

  // -- buffers
  struct Buffers {
    gtp::Buffer transformBuffer{};
    gtp::Buffer g_nodes_buffer{};
    gtp::Buffer g_nodes_indices{};
    gtp::Buffer blas_scratch{};
    gtp::Buffer second_blas_scratch{};
    gtp::Buffer tlas_scratch{};
    gtp::Buffer tlas_instancesBuffer{};
    gtp::Buffer ubo{};
    gtp::Buffer colorIDImageBuffer{};
  };

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
    std::vector<gtp::Particle*> particle;
  };

  Utilities_Renderer::ShaderBindingTableData shaderBindingTableData{};

  // -- top level acceleration structure
  Utilities_AS::AccelerationStructure TLAS{};

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- storage image
  Utilities_AS::StorageImage storageImage{};

  // -- storage image
  Utilities_AS::StorageImage colorIDStorageImage{};

  // -- uniform data
  Utilities_Renderer::UniformData uniformData{};

  // -- bottom level acceleration structures
  std::vector<Utilities_AS::BLASData *> bottomLevelAccelerationStructures;

  // -- TLAS data
  Utilities_AS::TLASData tlasData{};

  // -- Buffers
  Buffers buffers;

  // -- assets
  Assets assets{};

  // -- pipeline data
  Utilities_Renderer::PipelineData pipelineData{};

  // -- shader
  gtp::Shader shader;

  // -- compute
  std::vector<ComputeVertex *> gltfCompute;

  // -- constructor
  MainRenderer();

  // -- init constructor
  MainRenderer(EngineCore *pEngineCore);

  // -- init class function
  void Init_MainRenderer(EngineCore *pEngineCore);

  // -- load model
  void LoadModel(std::string filename, uint32_t fileLoadingFlags = 0,
                 Utilities_Renderer::ModelLoadingFlags modelLoadingFlags =
                     Utilities_Renderer::ModelLoadingFlags::None,
                 Utilities_UI::TransformMatrices *pTransformMatrices = nullptr);

  // -- load particle
  void LoadParticle(
    std::string filename, uint32_t fileLoadingFlags,
    Utilities_Renderer::ModelLoadingFlags modelLoadingFlags,
    Utilities_UI::TransformMatrices* pTransformMatrices);

  // -- load assets
  void LoadAssets();

  // -- load gltf compute
  void LoadGltfCompute(gtp::Model *pModel);

  // -- create BLAS
  void CreateBLAS(gtp::Model *pModel);

  // -- create top level acceleration structure
  void CreateTLAS();

  // -- handle particle tlas instances
  void HandleParticleInstancesTLAS(int particleIdx);

  // -- create storage image
  void CreateStorageImages();

  // -- create color id image buffer
  void CreateColorIDImageBuffer();

  // -- retrieve object id from image
  // -- copy object id image to buffer
  void RetrieveObjectIDFromImage();

  // -- create uniform buffer
  void CreateUniformBuffer();

  // -- update uniform buffer
  void UpdateUniformBuffer(float deltaTime, glm::vec4 lightPosition);

  // -- create ray tracing pipeline
  void CreateRayTracingPipeline();

  // -- update ray tracing pipeline
  void UpdateRayTracingPipeline();

  // -- create shader binding tables
  void CreateShaderBindingTable();

  // -- create descriptor set
  void CreateDescriptorSet();

  // -- setup buffer region addresses
  void SetupBufferRegionAddresses();

  // -- build command buffers
  void BuildCommandBuffers();

  // -- rebuild command buffers
  void RebuildCommandBuffers(int frame, bool showObjectColorID);

  // -- update BLAS
  void UpdateBLAS();

  // -- update TLAS
  void UpdateTLAS();

  // -- create geometry nodes buffer
  void CreateGeometryNodesBuffer();

  // -- update geometry nodes buffer
  void UpdateGeometryNodesBuffer(gtp::Model *pModel);

  // -- delete model
  void DeleteModel(gtp::Model *pModel);

  // -- update descriptor set
  void UpdateDescriptorSet();

  // -- handle window resize
  void HandleResize();

  // -- set model data from ui
  void SetModelData(Utilities_UI::ModelData *pModelData);

  // -- destroy class objects
  void Destroy_MainRenderer();
};
