#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

#define PARTICLE_COUNT 256 * 1024

namespace gtp {

class Particle {
public:
  // -- particle attributes
  struct Attributes {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 gradientPos;
  };

  // -- buffers
  struct Buffers {
    gtp::Buffer ssboStaging{};
    gtp::Buffer ssbo{};
  };

  Attributes attributes{};
  Buffers buffers{};

  // -- shader
  gtp::Shader shader;

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- default ctor
  Particle();

  // -- init ctor
  Particle(EngineCore *corePtr);

  // -- init func
  void InitParticle(EngineCore *corePtr);

  // -- destroy particle
  void DestroyParticle();

  // -- create ssbo
  void CreateParticleStorageBuffer();

};

} // namespace gtp
