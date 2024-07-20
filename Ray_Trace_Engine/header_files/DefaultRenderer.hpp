#pragma once
#include <RenderBase.hpp>

namespace gtp {

class DefaultRenderer : private RenderBase {
 private:
 public:
  // -- constructor
  DefaultRenderer(EngineCore* engineCorePtr);

  // -- destroy
  void Destroy();

  void Resize();

  Utilities_UI::ModelData* pModelData();

  void ModelDataSet(Utilities_UI::ModelData* pModelData);

  void ObjectID(int mousePosX, int mousePosY);

  void LoadNewModel(gtp::FileLoadingFlags loadingFlags);

  void HandleDeleteModel();

  std::vector<VkCommandBuffer> ComputeCommands(int frame);

  void UpdateShaderBuffers(float deltaTime, float timer, glm::vec4 lightPosition);

  void GraphicsCommands(int frame, bool showObjectColorID);
};

}  // namespace gtp
