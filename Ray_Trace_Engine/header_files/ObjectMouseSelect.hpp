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

// handles buffers and methods related to renderer object selection
class ObjectMouseSelect {
private:
  /* vars */
  
  // core ptr
  EngineCore *pEngineCore = nullptr;

  // -- storage image
  Utilities_AS::StorageImage colorIDStorageImage{};

  //Buffers
  struct Buffers {
    gtp::Buffer colorIDImageBuffer{};
  };

  Buffers buffers{};

  /* funcs */

  // -- init func
  void InitObjectMouseSelect(EngineCore *engineCorePtr);

  // -- create color id image buffer
  void CreateColorIDImageBuffer();

  // -- create color id storage image
  void CreateColorIDStorageImage();


public:
  // -- base ctor
  ObjectMouseSelect();

  // -- init ctor
  ObjectMouseSelect(EngineCore *engineCorePtr);

  // -- destroy
  void DestroyObjectMouseSelect();

};
} // namespace gtp
