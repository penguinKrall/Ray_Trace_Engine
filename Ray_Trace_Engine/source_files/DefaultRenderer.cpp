#include "DefaultRenderer.hpp"

gtp::DefaultRenderer::DefaultRenderer(EngineCore* engineCorePtr)
    : RenderBase(engineCorePtr) {}

void gtp::DefaultRenderer::Destroy() {
  // destroy base class resources
  this->DestroyRenderBase();
}
