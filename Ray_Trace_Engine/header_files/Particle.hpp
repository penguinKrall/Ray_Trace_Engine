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
  // -- particle "vertex"
  struct Vertex {
    glm::vec2 pos;
    glm::vec2 vel;
    glm::vec4 gradientPos;
  };

  // -- compute shader data
  struct ComputeBlock {
    float deltaT;
    float destX;
    float destY;
    int32_t particleCount = PARTICLE_COUNT;
  };

  // -- buffers
  struct Buffers {
    gtp::Buffer vertexSSBOStaging{};
    gtp::Buffer vertexSSBO{};
    gtp::Buffer computeBlockSSBO{};
  };

  // -- particle compute block struct
  ComputeBlock computeBlock{};

  // -- particle buffers
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

  // -- create vertex ssbo
  void CreateVertexStorageBuffer();

  // -- create compute block ssbo
  void CreateComputeBlockStorageBuffer();

  // -- update compute block ssbo
  void UpdateComputeBlockSSBO(float deltaTime, float timer);

};

} // namespace gtp
