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

#define VK_GLTF_MATERIAL_IDS

namespace gtp {
class RenderBase {
 private:
  /*	base class private data structures	*/
  // -- buffers
  struct Buffers {
    gtp::Buffer transformBuffer{};
    gtp::Buffer g_nodes_buffer{};
    gtp::Buffer g_nodes_indices{};
    gtp::Buffer blas_scratch{};
    gtp::Buffer tlas_scratch{};
    gtp::Buffer tlas_instancesBuffer{};
    gtp::Buffer ubo{};
  };
  Buffers buffers{};

  /*	base class private variables	*/
  // bottom level acceleration structure instances
  std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

  // -- geometry node vector
  std::vector<Utilities_AS::GeometryNode> geometryNodeBuf;

  // -- geometry node index vector
  std::vector<Utilities_AS::GeometryIndex> geometryIndexBuf;

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

    void LoadDefaultAssets(EngineCore* engineCorePtr);
    void DestroyDefaultAssets(EngineCore* engineCorePtr);
  };

  Assets assets{};

 public:
  /*	base class public variables	*/

  /*	base class public functions	*/
  // default constructor
  RenderBase(EngineCore* engineCorePtr);

  void DestroyRenderBase();
};
}  // namespace gtp
