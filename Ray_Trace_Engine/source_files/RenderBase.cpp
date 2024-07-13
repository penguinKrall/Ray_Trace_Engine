#include "RenderBase.hpp"

/*  initialize function */
void gtp::RenderBase::InitializeRenderBase(EngineCore* engineCorePtr) {
  // initialize core pointer
  this->pEngineCore = engineCorePtr;

  // initialize tools
  this->tools.InitializeTools(engineCorePtr);

  // load default assets
  this->assets.LoadDefaultAssets(engineCorePtr);

  // gtp::Utilities_EngCore::ListFilesInDirectory(
  //     "assets/textures/cubemaps/industrial_sunset_checker_ground");
}

/*  initialize constructor  */
gtp::RenderBase::RenderBase(EngineCore* engineCorePtr) {
  // call initialize function
  this->InitializeRenderBase(engineCorePtr);
}

/*  destroy the base class resources  */
void gtp::RenderBase::DestroyRenderBase() {
  // -- shaders
  this->tools.shader->DestroyShader();
  // -- object mouse select
  this->tools.objectMouseSelect->DestroyObjectMouseSelect();
  // -- assets
  this->assets.DestroyDefaultAssets(this->pEngineCore);
}

void gtp::RenderBase::Tools::InitializeTools(EngineCore* engineCorePtr) {
  // initialize object select
  this->objectMouseSelect = new gtp::ObjectMouseSelect(engineCorePtr);
  // shader
  shader = new gtp::Shader(engineCorePtr);
}

void gtp::RenderBase::Assets::LoadDefaultAssets(EngineCore* engineCorePtr) {
  // cubemap and default textures
  this->cubemap = gtp::TextureLoader(engineCorePtr);
  this->cubemap.LoadCubemap(
      "/assets/textures/cubemaps/industrial_sunset_checker_ground");

  this->defaultTextures.push_back(this->cubemap);

  // update texture offset for shader
  this->textureOffset = static_cast<uint32_t>(this->defaultTextures.size());

  const uint32_t glTFLoadingFlags =
      gtp::FileLoadingFlags::PreTransformVertices |
      gtp::FileLoadingFlags::PreMultiplyVertexColors;

  //// -- load animation
  // Utilities_UI::TransformMatrices animatedTransformMatrices{};
  //
  // animatedTransformMatrices.rotate = glm::rotate(
  //     glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  //
  // animatedTransformMatrices.translate =
  //     glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f));
  //
  // animatedTransformMatrices.scale =
  //     glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

  // this->LoadModel(
  //   "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/"
  //   "assets/models/Fox2/Fox2.gltf",
  //   gtp::FileLoadingFlags::None,
  //   Utilities_Renderer::ModelLoadingFlags::Animated,
  //   &animatedTransformMatrices);
  //
  //  -- load scene
  // this->LoadModel(
  //   "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/"
  //   "desert_scene/testScene.gltf",
  //   gtp::FileLoadingFlags::None, Utilities_Renderer::ModelLoadingFlags::None,
  //   nullptr);
  //
  //// -- load water surface
  // this->LoadModel(
  //   "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/"
  //   "desert_scene/pool_water_surface/pool_water_surface.gltf",
  //   gtp::FileLoadingFlags::None,
  //   Utilities_Renderer::ModelLoadingFlags::SemiTransparent, nullptr);

  //// -- load particle
  // this->LoadParticle("", gtp::FileLoadingFlags::None,
  //                    Utilities_Renderer::ModelLoadingFlags::None, nullptr);

  for (int i = 0; i < this->modelData.animatedModelIndex.size(); i++) {
    std::cout << "\nanimated model index[" << i
              << "]: " << this->modelData.animatedModelIndex[i] << std::endl;
  }
  std::cout << "this->modelData.animatedModelIndex.size(): "
            << this->modelData.animatedModelIndex.size() << std::endl;
}

void gtp::RenderBase::Assets::DestroyDefaultAssets(EngineCore* engineCorePtr) {
  for (auto& defaultTex : this->defaultTextures) {
    defaultTex.DestroyTextureLoader();
  }
}
