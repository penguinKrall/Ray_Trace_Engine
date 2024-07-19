#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <ObjectMouseSelect.hpp>
#include <Particle.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
//#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>
#include <AccelerationStructures.hpp>

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
  EngineCore* pEngineCore = nullptr;

  /*	base class private functions	*/
  // initialize function
  void InitializeRenderBase(EngineCore* engineCorePtr);

  // class tools
  struct Tools {
    // object select class
    gtp::ObjectMouseSelect* objectMouseSelect;

    // shader
    gtp::Shader* shader;

    // -- shader binding table data
    Utilities_Renderer::ShaderBindingTableData shaderBindingTableData{};

    // tools initialize function
    void InitializeTools(EngineCore* engineCorePtr);
  };

  Tools tools{};

  struct Assets {
    uint32_t textureOffset = 0;
    std::vector<gtp::TextureLoader> defaultTextures;

    // models
    std::vector<gtp::Model*> models;
    Utilities_UI::ModelData modelData;
    Utilities_UI::LoadModelFlags loadModelFlags;

    // colored glass texture
    gtp::TextureLoader coloredGlassTexture;

    // cube map
    gtp::TextureLoader cubemap;

    // particle
    std::vector<gtp::Particle*> particle;

    // gltf compute -- animation compute
    std::vector<ComputeVertex*> gltfCompute;

    void LoadDefaultAssets(EngineCore* engineCorePtr);
    void DestroyDefaultAssets(EngineCore* engineCorePtr);
  };

  Assets assets{};

  // default color storage image
  Utilities_Renderer::StorageImage defaultColorStorageImage{};

  // -- pipeline data
  Utilities_Renderer::PipelineData pipelineData{};

  // color storage image for ray trace pipeline
  void CreateDefaultColorStorageImage();

  // create default pipeline
  void CreateDefaultPipeline();

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

  void RebuildCommandBuffers(int frame, bool showObjectColorID);

 public:
  /*	base class public variables	*/

  /*	base class public functions	*/
  // default constructor
  RenderBase(EngineCore* engineCorePtr);

  void UpdateDefaultUniformBuffer(float deltaTime, glm::vec4 lightPosition);

  void DestroyRenderBase();
};
}  // namespace gtp
