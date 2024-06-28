#include "Particle.hpp"

gtp::Particle::Particle() {}

gtp::Particle::Particle(EngineCore *corePtr) { this->InitParticle(corePtr); }

void gtp::Particle::InitParticle(EngineCore *corePtr) {

  // -- initialize core pointer
  this->pEngineCore = corePtr;

  // -- initialize shader
  this->shader = gtp::Shader(pEngineCore);

  // -- create buffer particle storage buffer
  this->CreateParticleStorageBuffer();

  std::cout << " initialized particle!" << std::endl;
}

void gtp::Particle::DestroyParticle() {

  // buffers
  this->buffers.ssbo.destroy(this->pEngineCore->devices.logical);
  this->buffers.ssboStaging.destroy(this->pEngineCore->devices.logical);

  std::cout << " destroyed particle!" << std::endl;
}

void gtp::Particle::CreateParticleStorageBuffer() {
  std::default_random_engine rndEngine((unsigned)time(nullptr));
  std::uniform_real_distribution<float> rndDist(-1.0f, 1.0f);

  // Initial particle positions
  std::vector<Attributes> particleBuffer(PARTICLE_COUNT);
  for (auto &particle : particleBuffer) {
    particle.pos = glm::vec2(rndDist(rndEngine), rndDist(rndEngine));
    particle.vel = glm::vec2(0.0f);
    particle.gradientPos.x = particle.pos.x / 2.0f;
  }

  VkDeviceSize storageBufferSize = particleBuffer.size() * sizeof(Attributes);

  /* SSBO Staging Buffer */
  // -- init ssbo staging buffer and fill with particle attribute data
  this->buffers.ssboStaging.bufferData.bufferName =
      "gtp::Particle_ssbo_staging_buffer";
  this->buffers.ssboStaging.bufferData.bufferMemoryName =
      "gtp::Particle_ssbo_staging_bufferMemory";

  // -- create ssbo staging buffer
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &this->buffers.ssboStaging, storageBufferSize, particleBuffer.data());

  /* SSBO */
  // -- init ssbo and fill with particle attribute data
  this->buffers.ssbo.bufferData.bufferName = "gtp::Particle_ssbo_buffer";
  this->buffers.ssbo.bufferData.bufferMemoryName =
      "gtp::Particle_ssbo_bufferMemory";

  // -- create ssbo
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->buffers.ssbo,
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
  vkCmdCopyBuffer(cmdBuffer, this->buffers.ssboStaging.bufferData.buffer,
                  this->buffers.ssbo.bufferData.buffer, 1, &copyRegion);

  // submit temporary command buffer and destroy command buffer/memory
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);
}
