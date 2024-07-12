#include "RenderBase.hpp"

/*  initialize function */
void gtp::RenderBase::InitializeRenderBase(EngineCore* engineCorePtr) {
  // initialize core pointer
  this->pEngineCore = engineCorePtr;

  // initialize tools
  this->tools.InitializeTools(engineCorePtr);
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
}

void gtp::RenderBase::Tools::InitializeTools(EngineCore* engineCorePtr) {
  // initialize object select
  this->objectMouseSelect = new gtp::ObjectMouseSelect(engineCorePtr);
  // shader
  shader = new gtp::Shader(engineCorePtr);
}
