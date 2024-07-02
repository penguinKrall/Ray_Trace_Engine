#include "Particle.hpp"

gtp::Particle::Particle() {}

gtp::Particle::Particle(EngineCore *corePtr) { this->InitParticle(corePtr); }

void gtp::Particle::InitParticle(EngineCore *corePtr) {

  // -- initialize core pointer
  this->pEngineCore = corePtr;

  // -- initialize shader
  this->shader = gtp::Shader(pEngineCore);

  // -- create buffer particle storage buffer
  this->CreateVertexStorageBuffer();

  // -- create buffer particle storage buffer
  this->CreateComputeBlockStorageBuffer();

  std::cout << " initialized particle!" << std::endl;
}

void gtp::Particle::DestroyParticle() {

  // buffers
  this->buffers.computeBlockSSBO.destroy(this->pEngineCore->devices.logical);
  this->buffers.vertexSSBO.destroy(this->pEngineCore->devices.logical);
  this->buffers.vertexSSBOStaging.destroy(this->pEngineCore->devices.logical);

  std::cout << " destroyed particle!" << std::endl;
}

void gtp::Particle::CreateVertexStorageBuffer() {
  std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);

  // Initial particle positions
  std::vector<gtp::Particle::Vertex> particleBuffer(PARTICLE_COUNT);
  for (auto &particle : particleBuffer) {
    particle.pos = glm::vec2(rndDist(rndEngine), rndDist(rndEngine));
    particle.vel = glm::vec2(0.0f);
    particle.gradientPos.x = particle.pos.x / 2.0f;
  }

  VkDeviceSize storageBufferSize =
      particleBuffer.size() * sizeof(gtp::Particle::Vertex);

  /* SSBO Staging Buffer */
  // -- init ssbo staging buffer and fill with particle attribute data
  this->buffers.vertexSSBOStaging.bufferData.bufferName =
      "gtp::Particle_ssbo_staging_buffer";
  this->buffers.vertexSSBOStaging.bufferData.bufferMemoryName =
      "gtp::Particle_ssbo_staging_bufferMemory";

  // -- create ssbo staging buffer
  this->pEngineCore->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                      VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                  &this->buffers.vertexSSBOStaging,
                                  storageBufferSize, particleBuffer.data());

  /* SSBO */
  // -- init ssbo and fill with particle attribute data
  this->buffers.vertexSSBO.bufferData.bufferName = "gtp::Particle_ssbo_buffer";
  this->buffers.vertexSSBO.bufferData.bufferMemoryName =
      "gtp::Particle_ssbo_bufferMemory";

  // -- create ssbo
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->buffers.vertexSSBO,
      storageBufferSize, nullptr);

  /* Copy From Staging Buffer To SSBO */
  // -- one time submit command buffer and copy buffer command
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // copy region
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = storageBufferSize;

  // vulkan api copy command
  vkCmdCopyBuffer(cmdBuffer, this->buffers.vertexSSBOStaging.bufferData.buffer,
                  this->buffers.vertexSSBO.bufferData.buffer, 1, &copyRegion);

  // submit temporary command buffer and destroy command buffer/memory
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);
}

void gtp::Particle::CreateComputeBlockStorageBuffer() {

  // buffer size
  VkDeviceSize computeBlockSSBOSize = sizeof(gtp::Particle::ComputeBlock);

  // -- init compute block storage buffer and fill with particle attribute data
  this->buffers.computeBlockSSBO.bufferData.bufferName =
      "gtp::Particle_compute_block_storage_buffer";
  this->buffers.computeBlockSSBO.bufferData.bufferMemoryName =
      "gtp::Particle_compute_block_storage_bufferMemory";

  // -- create compute block storage buffer
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &this->buffers.computeBlockSSBO, computeBlockSSBOSize, nullptr);

  // -- map buffer memory
  validate_vk_result(buffers.computeBlockSSBO.map(
      this->pEngineCore->devices.logical, computeBlockSSBOSize, 0));

  // -- update buffer
  this->UpdateComputeBlockSSBO(0.0f, 0.0f);
}

void gtp::Particle::UpdateComputeBlockSSBO(float deltaTime, float timer) {

  // buffer size
  VkDeviceSize computeBlockSSBOSize = sizeof(gtp::Particle::ComputeBlock);

  // update delta time
  computeBlock.deltaT = timer * 2.5f;

  // pos attractor x position
  computeBlock.destX = sin(glm::radians(timer * 360.0f)) * 0.75f;

  // pos attractor y position
  computeBlock.destY = 0.0f;

  // copy particle compute block to storage buffer
  memcpy(buffers.computeBlockSSBO.bufferData.mapped, &computeBlock,
         computeBlockSSBOSize);
}
