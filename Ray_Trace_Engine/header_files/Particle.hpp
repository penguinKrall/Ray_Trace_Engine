#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <Shader.hpp>
#include <Sphere.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

#define PARTICLE_COUNT 256 * 1024

namespace gtp {

class Particle : private Sphere {
public:

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

  // -- default ctor
  Particle();

  // -- init ctor
  Particle(EngineCore *corePtr);

  // -- destroy particle
  void DestroyParticle();

private:
  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- particle compute block struct
  ComputeBlock computeBlock{};

  // -- particle buffers
  Buffers buffers{};

  // -- shader
  gtp::Shader shader;

  // -- pipeline data struct
  struct PipelineData {
    VkDescriptorPool descriptorPool{};
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSet descriptorSet{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
  };

  PipelineData pipelineData{};

  // -- bottom level acceleration structure/data
  Utilities_AS::BLASData blasData;
  Utilities_AS::AccelerationStructure BLAS;

  // -- init func
  void InitParticle(EngineCore *corePtr);

  // -- create vertex ssbo
  void CreateVertexStorageBuffer();

  // -- create compute block ssbo
  void CreateComputeBlockStorageBuffer();

  // -- update compute block ssbo
  void UpdateComputeBlockSSBO(float deltaTime, float timer);

  // -- create compute pipeline
  void CreateComputePipeline();

  // -- create particle BLAS
  void CreateParticleBLAS(
    std::vector<Utilities_AS::GeometryNode>* geometryNodeBuf,
    std::vector<Utilities_AS::GeometryIndex>* geometryIndexBuf,
    uint32_t textureOffset);

};

} // namespace gtp
