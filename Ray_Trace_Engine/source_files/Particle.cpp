#include "Particle.hpp"

gtp::Particle::Particle() {}

gtp::Particle::Particle(EngineCore *corePtr) { this->InitParticle(corePtr); }

VkCommandBuffer gtp::Particle::RecordComputeCommands(int frame) {

  // compute command buffer begin info
  VkCommandBufferBeginInfo computeBeginInfo{};
  computeBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  // reset command buffer
  validate_vk_result(vkResetCommandBuffer(this->commandBuffers[frame],
                                          /*VkCommandBufferResetFlagBits*/ 0));

  // begin command buffer
  validate_vk_result(
      vkBeginCommandBuffer(this->commandBuffers[frame], &computeBeginInfo));

  // bind pipeline
  vkCmdBindPipeline(this->commandBuffers[frame], VK_PIPELINE_BIND_POINT_COMPUTE,
                    this->pipelineData.pipeline);

  // bind descriptor set
  vkCmdBindDescriptorSets(this->commandBuffers[frame],
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          this->pipelineData.pipelineLayout, 0, 1,
                          &this->pipelineData.descriptorSet, 0, nullptr);

  // dispatch compute shader
  vkCmdDispatch(this->commandBuffers[frame],
                (static_cast<uint32_t>(this->sphereModel->vertexCount) + 255) /
                    256,
                1, 1);

  // end compute command buffer
  validate_vk_result(vkEndCommandBuffer(this->commandBuffers[frame]));

  return this->commandBuffers[frame];
}

gtp::Model* gtp::Particle::ParticleModel() {
  return this->sphereModel;
}

void gtp::Particle::DestroyParticle() {

  // descriptor pool
  vkDestroyDescriptorPool(this->pEngineCore->devices.logical,
                          this->pipelineData.descriptorPool, nullptr);

  // shaders
  this->shader.DestroyShader();

  // pipeline and layout
  vkDestroyPipelineLayout(this->pEngineCore->devices.logical,
                          this->pipelineData.pipelineLayout, nullptr);
  vkDestroyPipeline(this->pEngineCore->devices.logical,
                    this->pipelineData.pipeline, nullptr);

  // descriptor set layout
  vkDestroyDescriptorSetLayout(this->pEngineCore->devices.logical,
                               this->pipelineData.descriptorSetLayout, nullptr);

  // model
  this->sphereModel->destroy(this->pEngineCore->devices.logical);

  // buffers
  this->buffers.computeBlockSSBO.destroy(this->pEngineCore->devices.logical);
  this->buffers.vertexInputSSBO.destroy(this->pEngineCore->devices.logical);
  this->buffers.vertexOutputSSBO.destroy(this->pEngineCore->devices.logical);
  this->buffers.indexSSBO.destroy(this->pEngineCore->devices.logical);

  std::cout << " destroyed particle!" << std::endl;
}

void gtp::Particle::InitParticle(EngineCore *corePtr) {

  // initialize core pointer
  this->pEngineCore = corePtr;

  // initialize shader
  this->shader = gtp::Shader(pEngineCore);

  // model instance pointer to initialize and add to list
  this->sphereModel = new gtp::Model();

  // model index
  // auto modelIdx = static_cast<int>(this->assets.models.size());

  // -- Load From File ---- gtp::Model function
  this->sphereModel->loadFromFile(
      "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/"
      "assets/models/plane/plane.gltf",
      pEngineCore, pEngineCore->queue.graphics);

  // create particle vertex storage buffer
  this->CreateVertexStorageBuffer();

  // create particle index storage buffer
  this->CreateIndexStorageBuffer();

  // create buffer particle storage buffer
  this->CreateComputeBlockStorageBuffer();

  // create particle compute pipeline
  this->CreateComputePipeline();

  // create particle compute descriptor set
  this->CreateDescriptorSet();

  // create particle compute command buffers
  this->CreateCommandBuffers();

  std::cout << " initialized particle!" << std::endl;
}

void gtp::Particle::InstanceModel() {}

void gtp::Particle::CreateVertexStorageBuffer() {

  VkDeviceSize vertexStorageBufferSize =
      this->sphereModel->vertexCount * sizeof(gtp::Model::Vertex);

  //// init ssbo staging buffer and fill with particle attribute data
  // this->buffers.vertexSSBOStaging.bufferData.bufferName =
  //   "gtp::Particle_vertex_ssbo_staging_buffer";
  // this->buffers.vertexSSBOStaging.bufferData.bufferMemoryName =
  //   "gtp::Particle_vertex_ssbo_staging_bufferMemory";
  //
  //// create ssbo staging buffer
  // this->pEngineCore->CreateBuffer(
  //   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
  //   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
  //   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
  //   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  //   &this->buffers.vertexSSBOStaging, vertexStorageBufferSize,
  //  this->sphereModel->loaderInfo.vertexBuffer);

  // init input ssbo and fill with particle attribute data
  this->buffers.vertexInputSSBO.bufferData.bufferName =
      "gtp::Particle_input_ssbo_buffer";
  this->buffers.vertexInputSSBO.bufferData.bufferMemoryName =
      "gtp::Particle_input_ssbo_bufferMemory";

  // create input ssbo
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->buffers.vertexInputSSBO,
      vertexStorageBufferSize, nullptr);

  // init output ssbo and fill with particle attribute data
  this->buffers.vertexOutputSSBO.bufferData.bufferName =
      "gtp::Particle_output_ssbo_buffer";
  this->buffers.vertexOutputSSBO.bufferData.bufferMemoryName =
      "gtp::Particle_output_ssbo_bufferMemory";

  // create output ssbo
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->buffers.vertexOutputSSBO,
      vertexStorageBufferSize, nullptr);

  // one time submit command buffer and copy buffer command
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // copy region
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = vertexStorageBufferSize;

  // vulkan api copy command
  // input
  vkCmdCopyBuffer(cmdBuffer, this->sphereModel->vertices.buffer,
                  this->buffers.vertexInputSSBO.bufferData.buffer, 1,
                  &copyRegion);

  // output
  vkCmdCopyBuffer(cmdBuffer, this->sphereModel->vertices.buffer,
                  this->buffers.vertexOutputSSBO.bufferData.buffer, 1,
                  &copyRegion);

  // submit temporary command buffer and destroy command buffer/memory
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);
}

void gtp::Particle::CreateIndexStorageBuffer() {

  // index storage buffer size
  VkDeviceSize indexStorageBufferSize =
      this->sphereModel->indexCount * sizeof(uint32_t);

  //// index staging buffer
  // this->buffers.indexSSBOStaging.bufferData.bufferName =
  //   "gtp::Particle_index_ssbo_staging_buffer";
  // this->buffers.indexSSBOStaging.bufferData.bufferMemoryName =
  //   "gtp::Particle_index_ssbo_staging_bufferMemory";
  //
  //// create ssbo staging buffer
  // this->pEngineCore->CreateBuffer(
  //   VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
  //   VK_BUFFER_USAGE_TRANSFER_DST_BIT,
  //   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
  //   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
  //   &this->buffers.indexSSBOStaging, indexStorageBufferSize,
  //   this->SphereIndices().data());

  // index buffer
  // init input ssbo and fill with particle attribute data
  this->buffers.indexSSBO.bufferData.bufferName =
      "gtp::Particle_index_ssbo_buffer";
  this->buffers.indexSSBO.bufferData.bufferMemoryName =
      "gtp::Particle_index_ssbo_bufferMemory";

  // create input ssbo
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->buffers.indexSSBO,
      indexStorageBufferSize, nullptr);

  // one time submit command buffer and copy buffer command
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // copy region
  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = indexStorageBufferSize;

  // vulkan api copy command
  vkCmdCopyBuffer(cmdBuffer, this->sphereModel->indices.buffer,
                  this->buffers.indexSSBO.bufferData.buffer, 1, &copyRegion);

  // submit temporary command buffer and destroy command buffer/memory
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);
}

void gtp::Particle::CreateComputeBlockStorageBuffer() {

  // buffer size
  VkDeviceSize computeBlockSSBOSize = sizeof(gtp::Particle::ComputeBlock);

  // init compute block storage buffer and fill with particle attribute data
  this->buffers.computeBlockSSBO.bufferData.bufferName =
      "gtp::Particle_compute_block_storage_buffer";
  this->buffers.computeBlockSSBO.bufferData.bufferMemoryName =
      "gtp::Particle_compute_block_storage_bufferMemory";

  // create compute block storage buffer
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &this->buffers.computeBlockSSBO, computeBlockSSBOSize, nullptr);

  // map buffer memory
  validate_vk_result(buffers.computeBlockSSBO.map(
      this->pEngineCore->devices.logical, computeBlockSSBOSize, 0));

  // update buffer
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

void gtp::Particle::CreateComputePipeline() {

  // descriptor binding
  VkDescriptorSetLayoutBinding inputStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  VkDescriptorSetLayoutBinding outputStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  VkDescriptorSetLayoutBinding transformsStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  // array of bindings
  std::vector<VkDescriptorSetLayoutBinding> bindings(
      {inputStorageBufferLayoutBinding, outputStorageBufferLayoutBinding,
       transformsStorageBufferLayoutBinding});

  // descriptor set layout create info
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount =
      static_cast<uint32_t>(bindings.size());
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();

  // create descriptor set layout
  pEngineCore->add(
      [this, &descriptorSetLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorSetLayout(
            &descriptorSetLayoutCreateInfo, nullptr,
            &pipelineData.descriptorSetLayout);
      },
      "particle_compute_DescriptorSetLayout");

  // pipeline layout create info
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

  // create pipeline layout
  pEngineCore->add(
      [this, &pipelineLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreatePipelineLayout(
            &pipelineLayoutCreateInfo, nullptr, &pipelineData.pipelineLayout);
      },
      "particle_compute_pipelineLayout");

  // project directory for loading shader modules
  std::filesystem::path projDirectory = std::filesystem::current_path();

  // Setup ray tracing shader groups
  VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo;

  computeShaderStageCreateInfo = shader.loadShader(
      projDirectory.string() + "/shaders/compiled/particle_compute.comp.spv",
      VK_SHADER_STAGE_COMPUTE_BIT, "particle_compute_shader");
  VkComputePipelineCreateInfo computePipelineCreateInfo = {};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.stage = computeShaderStageCreateInfo;
  computePipelineCreateInfo.layout = this->pipelineData.pipelineLayout;

  validate_vk_result(vkCreateComputePipelines(
      pEngineCore->devices.logical, VK_NULL_HANDLE, 1,
      &computePipelineCreateInfo, nullptr, &this->pipelineData.pipeline));
}

void gtp::Particle::CreateDescriptorSet() {
  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3}};

  // descriptor pool create info
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
  descriptorPoolCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount =
      static_cast<uint32_t>(poolSizes.size());
  descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
  descriptorPoolCreateInfo.maxSets = 1;

  // create descriptor pool
  pEngineCore->add(
      [this, &descriptorPoolCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorPool(
            &descriptorPoolCreateInfo, nullptr,
            &this->pipelineData.descriptorPool);
      },
      "gltf_compute_DescriptorPool");

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
  descriptorSetAllocateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool = this->pipelineData.descriptorPool;
  descriptorSetAllocateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
  descriptorSetAllocateInfo.descriptorSetCount = 1;

  // create descriptor set
  pEngineCore->add(
      [this, &descriptorSetAllocateInfo]() {
        return pEngineCore->objCreate.VKAllocateDescriptorSet(
            &descriptorSetAllocateInfo, nullptr,
            &this->pipelineData.descriptorSet);
      },
      "gltf_compute_DescriptorSet");

  // storage input buffer descriptor info
  VkDescriptorBufferInfo storageInputBufferDescriptor{};
  storageInputBufferDescriptor.buffer =
      this->buffers.vertexInputSSBO.bufferData.buffer;
  storageInputBufferDescriptor.offset = 0;
  storageInputBufferDescriptor.range =
      static_cast<uint32_t>(this->sphereModel->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage input buffer descriptor write info
  VkWriteDescriptorSet storageInputBufferWrite{};
  storageInputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageInputBufferWrite.dstSet = pipelineData.descriptorSet;
  storageInputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  storageInputBufferWrite.dstBinding = 0;
  storageInputBufferWrite.pBufferInfo = &storageInputBufferDescriptor;
  storageInputBufferWrite.descriptorCount = 1;

  // storage output buffer descriptor info
  VkDescriptorBufferInfo storageOutputBufferDescriptor{};
  storageOutputBufferDescriptor.buffer =
      this->buffers.vertexOutputSSBO.bufferData.buffer;
  storageOutputBufferDescriptor.offset = 0;
  storageOutputBufferDescriptor.range =
      static_cast<uint32_t>(this->sphereModel->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage output buffer descriptor write info
  VkWriteDescriptorSet storageOutputBufferWrite{};
  storageOutputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageOutputBufferWrite.dstSet = pipelineData.descriptorSet;
  storageOutputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  storageOutputBufferWrite.dstBinding = 1;
  storageOutputBufferWrite.pBufferInfo = &storageOutputBufferDescriptor;
  storageOutputBufferWrite.descriptorCount = 1;

  // compute block descriptor info
  VkDescriptorBufferInfo computeBlockDescriptor{};
  computeBlockDescriptor.buffer =
      this->buffers.computeBlockSSBO.bufferData.buffer;
  computeBlockDescriptor.offset = 0;
  computeBlockDescriptor.range = sizeof(gtp::Particle::ComputeBlock);

  // compute block descriptor write info
  VkWriteDescriptorSet computeBlockBufferWrite{};
  computeBlockBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  computeBlockBufferWrite.dstSet = pipelineData.descriptorSet;
  computeBlockBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  computeBlockBufferWrite.dstBinding = 2;
  computeBlockBufferWrite.pBufferInfo = &computeBlockDescriptor;
  computeBlockBufferWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      storageInputBufferWrite, storageOutputBufferWrite,
      computeBlockBufferWrite};

  vkUpdateDescriptorSets(this->pEngineCore->devices.logical,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void gtp::Particle::CreateCommandBuffers() {
  commandBuffers.resize(frame_draws);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pEngineCore->commandPools.compute;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = frame_draws;

  validate_vk_result(vkAllocateCommandBuffers(
      pEngineCore->devices.logical, &allocInfo, commandBuffers.data()));
}
