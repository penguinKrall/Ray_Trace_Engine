#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <Shader.hpp>
// #include <Sphere.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

#define PARTICLE_COUNT 256 * 1024

namespace gtp {

class Particle {
 public:
  // -- default ctor
  Particle();

  // -- init ctor
  Particle(EngineCore *corePtr);

  // -- record commands
  VkCommandBuffer RecordComputeCommands(int frame);

  // -- get model
  gtp::Model *ParticleModel();

  // -- destroy particle
  void DestroyParticle();

  // -- initialize particle torus transforms
  VkTransformMatrixKHR ParticleTorusTransforms(int particleIdx, Utilities_UI::ModelData& modelData);

 private:
  gtp::Model *sphereModel = nullptr;

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
    gtp::Buffer vertexInputSSBO{};
    gtp::Buffer vertexOutputSSBO{};
    gtp::Buffer indexSSBOStaging{};
    gtp::Buffer indexSSBO{};
    gtp::Buffer computeBlockSSBO{};
  };

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- particle compute block struct
  ComputeBlock computeBlock{};

  // -- particle buffers
  Buffers buffers{};

  // -- command buffers
  std::vector<VkCommandBuffer> commandBuffers;

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

  // -- instance model
  void InstanceModel();

  // -- create particle vertex shader storage buffer objects
  void CreateVertexStorageBuffer();

  // -- creat particle index shader storage buffer
  void CreateIndexStorageBuffer();

  // -- create particle compute block ssbo
  void CreateComputeBlockStorageBuffer();

  // -- update particle compute block ssbo
  void UpdateComputeBlockSSBO(float deltaTime, float timer);

  // -- create particle compute pipeline
  void CreateComputePipeline();

  // -- create particle compute pipeline descriptor set
  void CreateDescriptorSet();

  // -- create particle compute pipeline command buffers
  void CreateCommandBuffers();

};

}  // namespace gtp
