#include "ComputeVertex.hpp"

ComputeVertex::ComputeVertex() {}

ComputeVertex::ComputeVertex(EngineCore *pEngineCore, gtp::Model *modelPtr) {

  Init_ComputeVertex(pEngineCore, modelPtr);

  CreateComputeBuffers();

  CreateTransformsBuffer();

  CreateComputePipeline();

  CreateDescriptorSets();

  CreateCommandBuffers();

  // UpdateTransformsBuffer(&this->transformMatrices);
}

void ComputeVertex::Init_ComputeVertex(EngineCore *pEngineCore,
                                       gtp::Model *modelPtr) {

  this->pEngineCore = pEngineCore;
  this->model = modelPtr;
  this->shader = gtp::Shader(pEngineCore);
}

void ComputeVertex::CreateTransformsBuffer() {

  VkDeviceSize transformsBufferSize = sizeof(Utilities_UI::TransformMatrices);
  this->transformsBuffer.bufferData.bufferName =
      "gltf_compute_transformsStorageBuffer_";
  this->transformsBuffer.bufferData.bufferMemoryName =
      "gltf_compute_transformsStorageBufferMemory_";

  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &this->transformsBuffer, transformsBufferSize, &transformMatrices);
}

void ComputeVertex::CreateComputeBuffers() {

  // input buffer
  this->storageInputBuffer.bufferData.bufferName =
      "gltf_compute_inputStorageBuffer_";
  this->storageInputBuffer.bufferData.bufferMemoryName =
      "gltf_compute_inputStorageBufferMemory_";

  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->storageInputBuffer,
      static_cast<uint32_t>(this->model->vertexCount) *
          sizeof(gtp::Model::Vertex),
      nullptr);

  // output buffer
  this->storageOutputBuffer.bufferData.bufferName =
      "gltf_compute_outputStorageBuffer_";
  this->storageOutputBuffer.bufferData.bufferMemoryName =
      "gltf_compute_outputStorageBufferMemory_";

  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->storageOutputBuffer,
      static_cast<uint32_t>(this->model->vertexCount) *
          sizeof(gtp::Model::Vertex),
      nullptr);

  // copy from existing model vertex buffer
  // create temporary command buffer
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = static_cast<uint32_t>(this->model->vertexCount) *
                    sizeof(gtp::Model::Vertex);

  vkCmdCopyBuffer(cmdBuffer, model->vertices.buffer,
                  this->storageInputBuffer.bufferData.buffer, 1, &copyRegion);
  vkCmdCopyBuffer(cmdBuffer, model->vertices.buffer,
                  this->storageOutputBuffer.bufferData.buffer, 1, &copyRegion);

  // submit temporary command buffer and destroy command buffer/memory
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // joint buffer
  std::vector<glm::mat4> transforms;

  for (auto &nodes : this->model->linearNodes) {
    if (nodes->mesh) {
      if (nodes->skin) {
        std::cout << "\n\n**GLTF_COMPUTE joint buffer" << std::endl;
        for (int i = 0; i < nodes->skin->joints.size(); i++) {
          std::cout << "node[" << i << "]name: " << nodes->skin->joints[i]->name
                    << std::endl;
          transforms.push_back(nodes->mesh->uniformBlock.jointMatrix[i]);
        }
      }
    }
  }

  size_t jointBufferSize = transforms.size() * sizeof(glm::mat4);

  this->jointBuffer.bufferData.bufferName = "gltf_compute_jointStorageBuffer_";
  this->jointBuffer.bufferData.bufferMemoryName =
      "gltf_compute_jointStorageBufferMemory_";

  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &this->jointBuffer, jointBufferSize, transforms.data());
}

void ComputeVertex::Destroy_ComputeVertex() {

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

  // storage buffers
  this->transformsBuffer.destroy(this->pEngineCore->devices.logical);
  this->jointBuffer.destroy(this->pEngineCore->devices.logical);
  this->storageInputBuffer.destroy(this->pEngineCore->devices.logical);
  this->storageOutputBuffer.destroy(this->pEngineCore->devices.logical);
}

void ComputeVertex::CreateComputePipeline() {

  // descriptor binding
  VkDescriptorSetLayoutBinding inputStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  VkDescriptorSetLayoutBinding outputStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  VkDescriptorSetLayoutBinding jointStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  VkDescriptorSetLayoutBinding transformsStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  // array of bindings
  std::vector<VkDescriptorSetLayoutBinding> bindings(
      {inputStorageBufferLayoutBinding, outputStorageBufferLayoutBinding,
       jointStorageBufferLayoutBinding, transformsStorageBufferLayoutBinding});

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
      "ComputeVertex_DescriptorSetLayout");

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
      "ComputeVertex_pipelineLayout");

  // project directory for loading shader modules
  std::filesystem::path projDirectory = std::filesystem::current_path();

  // Setup ray tracing shader groups
  VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo;

  computeShaderStageCreateInfo =
      shader.loadShader(projDirectory.string() +
                            "/shaders/compiled/main_renderer_compute.comp.spv",
                        VK_SHADER_STAGE_COMPUTE_BIT, "gltf_compute_shader");
  VkComputePipelineCreateInfo computePipelineCreateInfo = {};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.stage = computeShaderStageCreateInfo;
  computePipelineCreateInfo.layout = this->pipelineData.pipelineLayout;

  validate_vk_result(vkCreateComputePipelines(
      pEngineCore->devices.logical, VK_NULL_HANDLE, 1,
      &computePipelineCreateInfo, nullptr, &this->pipelineData.pipeline));
}

void ComputeVertex::CreateCommandBuffers() {

  commandBuffers.resize(frame_draws);

  VkCommandBufferAllocateInfo allocInfo{};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = pEngineCore->commandPools.compute;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = frame_draws;

  validate_vk_result(vkAllocateCommandBuffers(
      pEngineCore->devices.logical, &allocInfo, commandBuffers.data()));
}

void ComputeVertex::CreateDescriptorSets() {

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
  // storageInputBufferDescriptor.buffer = storageInputBuffer.bufferData.buffer;
  storageInputBufferDescriptor.buffer = this->model->vertices.buffer;
  storageInputBufferDescriptor.offset = 0;
  storageInputBufferDescriptor.range =
      static_cast<uint32_t>(this->model->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage input buffer descriptor write info
  VkWriteDescriptorSet storageInputBufferWrite{};
  storageInputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageInputBufferWrite.dstSet = pipelineData.descriptorSet;
  storageInputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  storageInputBufferWrite.dstBinding = 0;
  storageInputBufferWrite.pBufferInfo = &storageInputBufferDescriptor;
  storageInputBufferWrite.descriptorCount = 1;

  std::cout << "this->model->vertexBufferSize: "
            << this->model->vertexBufferSize << std::endl;
  // storage output buffer descriptor info
  VkDescriptorBufferInfo storageOutputBufferDescriptor{};
  // storageOutputBufferDescriptor.buffer =
  // this->model->vertices.buffer.bufferData.buffer;
  storageOutputBufferDescriptor.buffer = storageOutputBuffer.bufferData.buffer;
  storageOutputBufferDescriptor.offset = 0;
  storageOutputBufferDescriptor.range =
      static_cast<uint32_t>(this->model->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage output buffer descriptor write info
  VkWriteDescriptorSet storageOutputBufferWrite{};
  storageOutputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageOutputBufferWrite.dstSet = pipelineData.descriptorSet;
  storageOutputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  storageOutputBufferWrite.dstBinding = 1;
  storageOutputBufferWrite.pBufferInfo = &storageOutputBufferDescriptor;
  storageOutputBufferWrite.descriptorCount = 1;

  // joint buffer descriptor info
  VkDescriptorBufferInfo jointBufferDescriptor{};
  jointBufferDescriptor.buffer = jointBuffer.bufferData.buffer;
  jointBufferDescriptor.offset = 0;
  jointBufferDescriptor.range = jointBuffer.bufferData.size;

  // joint buffer descriptor write info
  VkWriteDescriptorSet jointBufferWrite{};
  jointBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  jointBufferWrite.dstSet = pipelineData.descriptorSet;
  jointBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  jointBufferWrite.dstBinding = 2;
  jointBufferWrite.pBufferInfo = &jointBufferDescriptor;
  jointBufferWrite.descriptorCount = 1;

  // transforms buffer descriptor info
  VkDescriptorBufferInfo transformsBufferDescriptor{};
  transformsBufferDescriptor.buffer = transformsBuffer.bufferData.buffer;
  transformsBufferDescriptor.offset = 0;
  transformsBufferDescriptor.range = transformsBuffer.bufferData.size;

  // transforms buffer descriptor write info
  VkWriteDescriptorSet transformsBufferWrite{};
  transformsBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  transformsBufferWrite.dstSet = pipelineData.descriptorSet;
  transformsBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  transformsBufferWrite.dstBinding = 3;
  transformsBufferWrite.pBufferInfo = &transformsBufferDescriptor;
  transformsBufferWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      storageInputBufferWrite, storageOutputBufferWrite, jointBufferWrite,
      transformsBufferWrite};

  vkUpdateDescriptorSets(this->pEngineCore->devices.logical,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

VkCommandBuffer ComputeVertex::RecordComputeCommands(int frame) {

  // compute command buffer begin info
  VkCommandBufferBeginInfo computeBeginInfo{};
  computeBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  // reset command buffer
  validate_vk_result(vkResetCommandBuffer(this->commandBuffers[frame],
    /*VkCommandBufferResetFlagBits*/ 0));

  // begin command buffer
  validate_vk_result(
    vkBeginCommandBuffer(this->commandBuffers[frame], &computeBeginInfo));

  vkCmdBindPipeline(this->commandBuffers[frame], VK_PIPELINE_BIND_POINT_COMPUTE,
                    this->pipelineData.pipeline);

  vkCmdBindDescriptorSets(this->commandBuffers[frame],
                          VK_PIPELINE_BIND_POINT_COMPUTE,
                          this->pipelineData.pipelineLayout, 0, 1,
                          &this->pipelineData.descriptorSet, 0, nullptr);

  vkCmdDispatch(this->commandBuffers[frame],
                (static_cast<uint32_t>(this->model->vertexCount) + 255) / 256,
                1, 1);

  // end compute command buffer
  validate_vk_result(vkEndCommandBuffer(this->commandBuffers[frame]));

  return this->commandBuffers[frame];

}

// std::vector<gtp::Model::Vertex> ComputeVertex::RetrieveBufferData() {
//
//	//void* data = nullptr;
//	//
//	//vkMapMemory(
//	//	this->pEngineCore->devices.logical,
// this->storageInputBuffer.bufferData.memory,
//	//	0, static_cast<uint32_t>(this->model->modelVertexBuffer.size())
//* sizeof(gtp::Vertex), 0, &data);
//	//
//	//std::vector<gtp::Vertex> tempVertex{};
//	//tempVertex.resize(this->model->modelVertexBuffer.size());
//	////std::vector<float> outputVector(this->count);
//	//
//	//memcpy(tempVertex.data(), data,
//(size_t)static_cast<uint32_t>(this->model->modelVertexBuffer.size()) *
// sizeof(gtp::Vertex));
//	//
//	//vkUnmapMemory(this->pEngineCore->devices.logical,
// this->storageInputBuffer.bufferData.memory);
//	//
//	//return tempVertex;
//
// }

void ComputeVertex::UpdateJointBuffer() {

  std::vector<glm::mat4> transforms;

  for (auto &nodes : this->model->linearNodes) {
    if (nodes->mesh) {
      if (nodes->skin) {
        for (int i = 0; i < nodes->skin->joints.size(); i++) {
          // std::cout << "node names: " << nodes->skin->joints[i]->name <<
          // std::endl;
          transforms.push_back(nodes->mesh->uniformBlock.jointMatrix[i]);
        }
      }
    }
  }

  size_t jointBufferSize = transforms.size() * sizeof(glm::mat4);

  this->jointBuffer.copyTo(transforms.data(), jointBufferSize);
}
