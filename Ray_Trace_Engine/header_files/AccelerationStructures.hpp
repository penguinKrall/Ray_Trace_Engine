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

namespace gtp {

class AccelerationStructures {
  /* vars */

  // core ptr
  EngineCore *pEngineCore = nullptr;

  /* funcs */

  // -- init func
  void InitAccelerationStructures(EngineCore *engineCorePtr);

public:
  // -- base ctor
  AccelerationStructures();

  // -- init ctor
  AccelerationStructures(EngineCore *engineCorePtr);

  // -- destroy
  void DestroyAccelerationStructures();
};

} // namespace gtp
