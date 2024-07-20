#include "DefaultRenderer.hpp"

gtp::DefaultRenderer::DefaultRenderer(EngineCore *engineCorePtr)
    : RenderBase(engineCorePtr) {}

void gtp::DefaultRenderer::Destroy() {
  // destroy base class resources
  this->DestroyRenderBase();
}

void gtp::DefaultRenderer::Resize() { this->HandleResize(); }

Utilities_UI::ModelData *gtp::DefaultRenderer::pModelData() {
  return this->GetModelData();
}

void gtp::DefaultRenderer::ModelDataSet(Utilities_UI::ModelData *pModelData) {
  this->SetModelData(pModelData);
}

void gtp::DefaultRenderer::ObjectID(int mousePosX, int mousePosY)
{
  this->RetrieveObjectID(mousePosX, mousePosY);
}

void gtp::DefaultRenderer::LoadNewModel(gtp::FileLoadingFlags loadingFlags)
{
  this->HandleLoadModel(loadingFlags);
}

void gtp::DefaultRenderer::HandleDeleteModel()
{
  this->DeleteModel();
}

std::vector<VkCommandBuffer> gtp::DefaultRenderer::ComputeCommands(int frame)
{
  return this->RecordCompute(frame);
}

void gtp::DefaultRenderer::UpdateShaderBuffers(float deltaTime, float timer, glm::vec4 lightPosition)
{
  this->UpdateAnimations(deltaTime);

  this->UpdateDefaultUniformBuffer(
    timer, lightPosition);

  this->RebuildAS();
}

void gtp::DefaultRenderer::GraphicsCommands(int frame, bool showObjectColorID)
{
  this->RebuildCommandBuffers(frame, showObjectColorID);
}
