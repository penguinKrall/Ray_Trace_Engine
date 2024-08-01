#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <ObjectMouseSelect.hpp>
#include <Particle.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
// #include <Utilities_AS.hpp>
#include <AccelerationStructures.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

#define VK_GLTF_MATERIAL_IDS

namespace gtp {
class RenderBase : private AccelerationStructures {
private:


  /*	base class private variables	and data structures*/
  // -- uniform data
  Utilities_Renderer::UniformData uniformData{};

  // -- buffers
  struct Buffers {
    gtp::Buffer ubo{};
  };

  Buffers buffers{};

  // core pointer
  EngineCore *pEngineCore = nullptr;

  /*	base class private functions	*/
  // initialize function
  void InitializeRenderBase(EngineCore *engineCorePtr);

  // class tools
  struct Tools {
    // object select class
    gtp::ObjectMouseSelect *objectMouseSelect;

    // shader
    gtp::Shader *shader;

    // -- shader binding table data
    Utilities_Renderer::ShaderBindingTableData shaderBindingTableData{};

    // tools initialize function
    void InitializeTools(EngineCore *engineCorePtr);
  };

  Tools tools{};

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

    // gltf compute -- animation compute
    std::vector<ComputeVertex *> gltfCompute;

    void LoadDefaultAssets(EngineCore *engineCorePtr);
    void DestroyDefaultAssets(EngineCore *engineCorePtr);
  };

  Assets assets{};

  // default color storage image
  struct StorageImages {
    // core pointer
    EngineCore *pEngineCore = nullptr;

    // default color 1 bit storage image
    Utilities_Renderer::StorageImage *defaultColor_1_bit{};

    // multisample
    // color
    Utilities_Renderer::StorageImage multisampleImage_8_bit{};
    // resolve
    Utilities_Renderer::StorageImage multisampleImageResolve_1_bit{};

    // default constructor
    explicit StorageImages(EngineCore *engineCorePtr) {
      this->pEngineCore = engineCorePtr;
      this->CreateStorageImages();
    }

    // color storage image for ray trace pipeline
    void CreateDefaultColorStorageImage();

    // create multisample
    //@brief creates 8 bit and 1 bit resolve
    void CreateMultisampleResources();

    // create storage images
    void CreateStorageImages();
  };

  StorageImages *storageImages;

  // -- pipeline data
  Utilities_Renderer::PipelineData pipelineData{};

  // create default pipeline
  void CreateDefaultRayTracingPipeline();

  // create shader binding table
  void CreateShaderBindingTable();

  // create default uniform buffer
  void CreateDefaultUniformBuffer();

  // create default descriptor set
  void CreateDefaultDescriptorSet();

  // setup buffer region device addresses
  void SetupBufferRegionAddresses();

  // build command buffers
  void CreateDefaultCommandBuffers();

  void
  SetupModelDataTransforms(Utilities_UI::TransformMatrices *pTransformMatrices);

  void LoadGltfCompute(gtp::Model *pModel);

  void UpdateDefaultRayTracingPipeline();

  void UpdateDefaultDescriptorSet();


public:
  /*	base class public variables	*/

  /*	base class public functions	*/
  // default constructor
  explicit RenderBase(EngineCore *engineCorePtr);

  // update default uniform buffer
  void UpdateDefaultUniformBuffer(float deltaTime, glm::vec4 lightPosition);

  // rebuild command buffers
  void RebuildCommandBuffers(int frame, bool showObjectColorID);

  // destroy render base class and resources
  void DestroyRenderBase();

  Utilities_UI::ModelData *GetModelData();

  // set assets model data struct // primary interface between renderer and ui
  // via engine
  void SetModelData(Utilities_UI::ModelData *pModelData);

  // retrieve object id
  //@brief uses object mouse select class color map and window input to find
  // which object is under mouse when LMB pressed
  void RetrieveObjectID(int posX, int posY);

  // load model
  void LoadModel(std::string filename, uint32_t fileLoadingFlags = 0,
                 uint32_t modelLoadingFlags =
                     Utilities_Renderer::ModelLoadingFlags::None,
                 Utilities_UI::TransformMatrices *pTransformMatrices = nullptr);

  // handle load model
  void HandleLoadModel(gtp::FileLoadingFlags loadingFlags);

  // handle resize
  void HandleResize();

  // record compute commands
  std::vector<VkCommandBuffer> RecordCompute(int frame);

  // update animateds
  void UpdateAnimations(float deltaTime);

  // -- delete model
  void DeleteModel();

  void RebuildAS();
};
} // namespace gtp
