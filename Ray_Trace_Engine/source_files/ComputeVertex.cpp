#include "ComputeVertex.hpp"

// create uniform buffer for compute
void ComputeVertex::CreateGeometryBuffer() {
  std::cout << "\nCompute Vertex Create Geometry Buffer\n";
  std::cout << "\tmodel:" << this->model->modelName << std::endl;

  // iterate through models linear nodes to check for mesh
  int i = 0;
  for (auto &node : this->model->linearNodes) {
    if (node->mesh) {
      // iterate through meshes to find primitives
      for (auto &primitive : node->mesh->primitives) {
        if (primitive->indexCount > 0) {
          geometryBufferData.geometries.resize(i + 1);

          geometryBufferData.geometries[i].textureIndexBaseColor =
              primitive->material.baseColorTexture
                  ? static_cast<int>(
                        primitive->material.baseColorTexture->index)
                  : -1;

          std::cout << "\ttextureIndexBaseColor:"
                    << geometryBufferData.geometries[i].textureIndexBaseColor
                    << std::endl;

          geometryBufferData.geometries[i].textureIndexOcclusion =
              primitive->material.occlusionTexture
                  ? static_cast<int>(
                        primitive->material.occlusionTexture->index)
                  : -1;

          std::cout << "\ttextureIndexOcclusion:"
                    << geometryBufferData.geometries[i].textureIndexOcclusion
                    << std::endl;

          geometryBufferData.geometries[i].textureIndexMetallicRoughness =
              primitive->material.metallicRoughnessTexture
                  ? static_cast<int>(
                        primitive->material.metallicRoughnessTexture->index)
                  : -1;

          std::cout
              << "\ttextureIndexMetallicRoughness:"
              << geometryBufferData.geometries[i].textureIndexMetallicRoughness
              << std::endl;

          geometryBufferData.geometries[i].textureIndexNormal =
              primitive->material.normalTexture
                  ? static_cast<int>(primitive->material.normalTexture->index)
                  : -1;

          std::cout << "\ttextureIndexNormal:"
                    << geometryBufferData.geometries[i].textureIndexNormal
                    << std::endl;

          geometryBufferData.geometries[i].firstVertex = primitive->firstVertex;

          std::cout << "\tfirstVertex:"
                    << geometryBufferData.geometries[i].firstVertex
                    << std::endl;

          geometryBufferData.geometries[i].vertexCount = primitive->vertexCount;

          std::cout << "\tvertexCount:"
                    << geometryBufferData.geometries[i].vertexCount << '\n'
                    << std::endl;

          ++i;
          //this->geometryBufferData.geometryCount = static_cast<double>(i);
        }
      }
    }
  }

  geometryBuffer.bufferData.bufferName = "compute_vertex_geometry_buffer";
  geometryBuffer.bufferData.bufferMemoryName =
      "compute_vertex_geometry_buffer_memory";

  // Calculate the total size required for the buffer
  VkDeviceSize totalSize =
      sizeof(Geometry) *
          static_cast<uint32_t>(geometryBufferData.geometries.size());

  //void *bufferData = malloc(totalSize);
  // Copy the geometry count to the buffer first
  //memcpy(bufferData, &geometryBufferData.geometryCount, sizeof(double));

  // Copy the geometry data to the buffer next
  //memcpy(static_cast<char *>(bufferData) + sizeof(double),
  //       geometryBufferData.geometries.data(),
  //       sizeof(Geometry) * geometryBufferData.geometries.size());

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                &geometryBuffer, totalSize,
                                geometryBufferData.geometries.data()) != VK_SUCCESS) {
    throw std::invalid_argument(
        "failed to create compute vertex geometry buffer!");
  }
}

ComputeVertex::ComputeVertex() {}

ComputeVertex::ComputeVertex(EngineCore *pEngineCore, gtp::Model *modelPtr) {

  this->Init_ComputeVertex(pEngineCore, modelPtr);

  this->CreateBuffers();

  this->CreateTransformMatrixBuffer();

  if (modelPtr->animations.size() > 0) {
    this->CreateAnimationComputePipeline();
    this->CreateAnimationPipelineDescriptorSet();
    std::cout << "created animation compute pipeine!" << std::endl;
  }

  else {
    this->CreateStaticComputePipeline();
    this->CreateStaticPipelineDescriptorSet();
    std::cout << "created static compute pipeine!" << std::endl;
  }

  CreateCommandBuffers();

  // UpdateTransformsBuffer(&this->transformMatrices);
}

void ComputeVertex::Init_ComputeVertex(EngineCore *pEngineCore,
                                       gtp::Model *modelPtr) {

  this->pEngineCore = pEngineCore;
  this->model = modelPtr;
  this->shader = gtp::Shader(pEngineCore);
  this->CreateGeometryBuffer();
}

void ComputeVertex::CreateTransformMatrixBuffer() {

  VkDeviceSize transformMatrixBufferSize =
      sizeof(Utilities_UI::TransformMatrices);
  this->transformMatrixBuffer.bufferData.bufferName =
      "compute_vertex_transformsStorageBuffer_";
  this->transformMatrixBuffer.bufferData.bufferMemoryName =
      "compute_vertex_transformsStorageBufferMemory_";

  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &this->transformMatrixBuffer, transformMatrixBufferSize,
      &transformMatrices);
}

void ComputeVertex::CreateBuffers() {

  /* vertex read buffer*/
  this->vertexReadBuffer.bufferData.bufferName =
      "compute_vertex_outputStorageBuffer_";
  this->vertexReadBuffer.bufferData.bufferMemoryName =
      "compute_vertex_outputStorageBufferMemory_";

  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &this->vertexReadBuffer,
      static_cast<uint32_t>(this->model->vertexCount) *
          sizeof(gtp::Model::Vertex),
      nullptr);

  // create one time submit command buffer and copy model vertex buffer to read
  // storage buffer
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  VkBufferCopy copyRegion{};
  copyRegion.srcOffset = 0;
  copyRegion.dstOffset = 0;
  copyRegion.size = static_cast<uint32_t>(this->model->vertexCount) *
                    sizeof(gtp::Model::Vertex);

  vkCmdCopyBuffer(cmdBuffer, model->vertices.buffer,
                  this->vertexReadBuffer.bufferData.buffer, 1, &copyRegion);

  // submit and destroy one time submit command buffer
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  /* joint buffer */
  if (!this->model->animations.empty()) {

    std::vector<glm::mat4> transforms;

    for (auto &nodes : this->model->linearNodes) {
      if (nodes->mesh) {
        if (nodes->skin) {
          std::cout << "\ncompute vertex joint buffer" << std::endl;
          for (int i = 0; i < nodes->skin->joints.size(); i++) {
            std::cout << "\tnode[" << i
                      << "]name: " << nodes->skin->joints[i]->name << std::endl;
            transforms.push_back(nodes->mesh->uniformBlock.jointMatrix[i]);
          }
        }
      }
    }

    size_t boneBufferSize = transforms.size() * sizeof(glm::mat4);

    this->boneBuffer.bufferData.bufferName =
        "compute_vertex_jointStorageBuffer";
    this->boneBuffer.bufferData.bufferMemoryName =
        "compute_vertex_jointStorageBufferMemory_";

    this->pEngineCore->CreateBuffer(
        VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &this->boneBuffer, boneBufferSize, transforms.data());
  }
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
  this->transformMatrixBuffer.destroy(this->pEngineCore->devices.logical);
  this->boneBuffer.destroy(this->pEngineCore->devices.logical);
  // this->modelVertexBuffer.destroy(this->pEngineCore->devices.logical);
  this->vertexReadBuffer.destroy(this->pEngineCore->devices.logical);
  this->geometryBuffer.destroy(this->pEngineCore->GetLogicalDevice());
}

void ComputeVertex::CreateAnimationComputePipeline() {

  // descriptor binding
  //'input' storage buffer - unchanging vertex data from when the model is
  // loaded
  VkDescriptorSetLayoutBinding inputStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  //'output' storage buffer - the modified version of the input buffer
  VkDescriptorSetLayoutBinding outputStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  // joint/bones buffer
  VkDescriptorSetLayoutBinding jointStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  // transform matrices
  VkDescriptorSetLayoutBinding transformsStorageBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  // uniform buffer layout binding
  VkDescriptorSetLayoutBinding geometryBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  // textures
  VkDescriptorSetLayoutBinding textureLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<uint32_t>(this->model->textures.size()),
          VK_SHADER_STAGE_COMPUTE_BIT, nullptr);

  // array of bindings
  std::vector<VkDescriptorSetLayoutBinding> bindings(
      {inputStorageBufferLayoutBinding, outputStorageBufferLayoutBinding,
       jointStorageBufferLayoutBinding, transformsStorageBufferLayoutBinding,
       geometryBufferLayoutBinding, textureLayoutBinding});

  // Unbound set
  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
  setLayoutBindingFlags.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
  setLayoutBindingFlags.bindingCount = static_cast<uint32_t>(bindings.size());
  std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
      0, 0, 0, 0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};

  setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

  // descriptor set layout create info
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount =
      static_cast<uint32_t>(bindings.size());
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();
  descriptorSetLayoutCreateInfo.pNext = &setLayoutBindingFlags;

  // create descriptor set layout
  pEngineCore->AddObject(
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
  pEngineCore->AddObject(
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
                        VK_SHADER_STAGE_COMPUTE_BIT, "compute_vertex_shader");
  VkComputePipelineCreateInfo computePipelineCreateInfo = {};
  computePipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  computePipelineCreateInfo.stage = computeShaderStageCreateInfo;
  computePipelineCreateInfo.layout = this->pipelineData.pipelineLayout;

  validate_vk_result(vkCreateComputePipelines(
      pEngineCore->devices.logical, VK_NULL_HANDLE, 1,
      &computePipelineCreateInfo, nullptr, &this->pipelineData.pipeline));
}

void ComputeVertex::CreateStaticComputePipeline() {
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

  // uniform buffer layout binding
  VkDescriptorSetLayoutBinding geometryBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT,
          nullptr);

  // textures
  VkDescriptorSetLayoutBinding textureLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          static_cast<uint32_t>(this->model->textures.size()),
          VK_SHADER_STAGE_COMPUTE_BIT, nullptr);

  // array of bindings
  std::vector<VkDescriptorSetLayoutBinding> bindings(
      {inputStorageBufferLayoutBinding, outputStorageBufferLayoutBinding,
       transformsStorageBufferLayoutBinding, geometryBufferLayoutBinding,
       textureLayoutBinding});

  // Unbound set
  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
  setLayoutBindingFlags.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
  setLayoutBindingFlags.bindingCount = static_cast<uint32_t>(bindings.size());
  std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
      0, 0, 0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};

  setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

  // descriptor set layout create info
  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount =
      static_cast<uint32_t>(bindings.size());
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();
  descriptorSetLayoutCreateInfo.pNext = &setLayoutBindingFlags;

  // create descriptor set layout
  pEngineCore->AddObject(
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
  pEngineCore->AddObject(
      [this, &pipelineLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreatePipelineLayout(
            &pipelineLayoutCreateInfo, nullptr, &pipelineData.pipelineLayout);
      },
      "ComputeVertex_pipelineLayout");

  // project directory for loading shader modules
  std::filesystem::path projDirectory = std::filesystem::current_path();

  // Setup ray tracing shader groups
  VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo;

  computeShaderStageCreateInfo = shader.loadShader(
      projDirectory.string() +
          "/shaders/compiled/main_renderer_static_compute.comp.spv",
      VK_SHADER_STAGE_COMPUTE_BIT, "gltf_static_compute_shader");
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

void ComputeVertex::CreateAnimationPipelineDescriptorSet() {

  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}};

  // descriptor pool create info
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
  descriptorPoolCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount =
      static_cast<uint32_t>(poolSizes.size());
  descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
  descriptorPoolCreateInfo.maxSets = 1;

  // create descriptor pool
  pEngineCore->AddObject(
      [this, &descriptorPoolCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorPool(
            &descriptorPoolCreateInfo, nullptr,
            &this->pipelineData.descriptorPool);
      },
      "compute_vertex_DescriptorPool");

  // variable descriptor count allocate info extension
  VkDescriptorSetVariableDescriptorCountAllocateInfoEXT
      variableDescriptorCountAllocInfo{};
  uint32_t variableDescCounts[] = {
      static_cast<uint32_t>(this->model->textures.size())};
  variableDescriptorCountAllocInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
  variableDescriptorCountAllocInfo.descriptorSetCount = 1;
  variableDescriptorCountAllocInfo.pDescriptorCounts = variableDescCounts;

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
  descriptorSetAllocateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool = this->pipelineData.descriptorPool;
  descriptorSetAllocateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
  descriptorSetAllocateInfo.descriptorSetCount = 1;
  descriptorSetAllocateInfo.pNext = &variableDescriptorCountAllocInfo;

  // create descriptor set
  pEngineCore->AddObject(
      [this, &descriptorSetAllocateInfo]() {
        return pEngineCore->objCreate.VKAllocateDescriptorSet(
            &descriptorSetAllocateInfo, nullptr,
            &this->pipelineData.descriptorSet);
      },
      "compute_vertex_DescriptorSet");

  // storage input buffer descriptor info
  VkDescriptorBufferInfo modelVertexBufferDescriptor{};
  modelVertexBufferDescriptor.buffer = this->model->vertices.buffer;
  modelVertexBufferDescriptor.offset = 0;
  modelVertexBufferDescriptor.range =
      static_cast<uint32_t>(this->model->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage input buffer descriptor write info
  VkWriteDescriptorSet modelVertexBufferWrite{};
  modelVertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  modelVertexBufferWrite.dstSet = pipelineData.descriptorSet;
  modelVertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  modelVertexBufferWrite.dstBinding = 0;
  modelVertexBufferWrite.pBufferInfo = &modelVertexBufferDescriptor;
  modelVertexBufferWrite.descriptorCount = 1;

  std::cout << "this->model->vertexBufferSize: "
            << this->model->vertexBufferSize << std::endl;
  // storage output buffer descriptor info
  VkDescriptorBufferInfo vertexReadBufferDescriptor{};
  vertexReadBufferDescriptor.buffer = vertexReadBuffer.bufferData.buffer;
  vertexReadBufferDescriptor.offset = 0;
  vertexReadBufferDescriptor.range =
      static_cast<uint32_t>(this->model->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage output buffer descriptor write info
  VkWriteDescriptorSet vertexReadBufferWrite{};
  vertexReadBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  vertexReadBufferWrite.dstSet = pipelineData.descriptorSet;
  vertexReadBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  vertexReadBufferWrite.dstBinding = 1;
  vertexReadBufferWrite.pBufferInfo = &vertexReadBufferDescriptor;
  vertexReadBufferWrite.descriptorCount = 1;

  // joint buffer descriptor info
  VkDescriptorBufferInfo boneBufferDescriptor{};
  boneBufferDescriptor.buffer = boneBuffer.bufferData.buffer;
  boneBufferDescriptor.offset = 0;
  boneBufferDescriptor.range = boneBuffer.bufferData.size;

  // joint buffer descriptor write info
  VkWriteDescriptorSet boneBufferWrite{};
  boneBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  boneBufferWrite.dstSet = pipelineData.descriptorSet;
  boneBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  boneBufferWrite.dstBinding = 2;
  boneBufferWrite.pBufferInfo = &boneBufferDescriptor;
  boneBufferWrite.descriptorCount = 1;

  // transforms buffer descriptor info
  VkDescriptorBufferInfo transformMatrixBufferDescriptor{};
  transformMatrixBufferDescriptor.buffer =
      transformMatrixBuffer.bufferData.buffer;
  transformMatrixBufferDescriptor.offset = 0;
  transformMatrixBufferDescriptor.range = transformMatrixBuffer.bufferData.size;

  // transforms buffer descriptor write info
  VkWriteDescriptorSet transformMatrixBufferWrite{};
  transformMatrixBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  transformMatrixBufferWrite.dstSet = pipelineData.descriptorSet;
  transformMatrixBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  transformMatrixBufferWrite.dstBinding = 3;
  transformMatrixBufferWrite.pBufferInfo = &transformMatrixBufferDescriptor;
  transformMatrixBufferWrite.descriptorCount = 1;

  // geometry descriptor info
  VkDescriptorBufferInfo geometryDescriptor{};
  geometryDescriptor.buffer = geometryBuffer.bufferData.buffer;
  geometryDescriptor.offset = 0;
  geometryDescriptor.range =
      static_cast<uint32_t>(geometryBufferData.geometries.size()) *
      sizeof(Geometry);

  // geometry descriptor write info
  VkWriteDescriptorSet geometryBufferWrite{};
  geometryBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  geometryBufferWrite.dstSet = pipelineData.descriptorSet;
  geometryBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  geometryBufferWrite.dstBinding = 4;
  geometryBufferWrite.pBufferInfo = &geometryDescriptor;
  geometryBufferWrite.descriptorCount = 1;

  // Image descriptors for the image array
  std::vector<VkDescriptorImageInfo> textureDescriptors{};

  for (int i = 0; i < this->model->textures.size(); i++) {
    VkDescriptorImageInfo descriptor{};
    descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor.sampler = this->model->textures[i].sampler;
    descriptor.imageView = this->model->textures[i].view;
    textureDescriptors.push_back(descriptor);
  }

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      modelVertexBufferWrite, vertexReadBufferWrite, boneBufferWrite,
      transformMatrixBufferWrite, geometryBufferWrite};

  if (!this->model->textures.empty()) {
    VkWriteDescriptorSet writeDescriptorImgArray{};
    writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorImgArray.dstBinding = 5;
    writeDescriptorImgArray.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorImgArray.descriptorCount =
        static_cast<uint32_t>(textureDescriptors.size());
    writeDescriptorImgArray.dstSet = this->pipelineData.descriptorSet;
    writeDescriptorImgArray.pImageInfo = textureDescriptors.data();
    writeDescriptorSets.push_back(writeDescriptorImgArray);
  }

  vkUpdateDescriptorSets(this->pEngineCore->devices.logical,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void ComputeVertex::CreateStaticPipelineDescriptorSet() {
  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 5},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 100}};

  // descriptor pool create info
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
  descriptorPoolCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount =
      static_cast<uint32_t>(poolSizes.size());
  descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
  descriptorPoolCreateInfo.maxSets = 1;

  // create descriptor pool
  pEngineCore->AddObject(
      [this, &descriptorPoolCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorPool(
            &descriptorPoolCreateInfo, nullptr,
            &this->pipelineData.descriptorPool);
      },
      "compute_vertex_DescriptorPool");

  // variable descriptor count allocate info extension
  VkDescriptorSetVariableDescriptorCountAllocateInfoEXT
      variableDescriptorCountAllocInfo{};
  uint32_t variableDescCounts[] = {
      static_cast<uint32_t>(this->model->textures.size())};
  variableDescriptorCountAllocInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
  variableDescriptorCountAllocInfo.descriptorSetCount = 1;
  variableDescriptorCountAllocInfo.pDescriptorCounts = variableDescCounts;

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
  descriptorSetAllocateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool = this->pipelineData.descriptorPool;
  descriptorSetAllocateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
  descriptorSetAllocateInfo.descriptorSetCount = 1;
  descriptorSetAllocateInfo.pNext = &variableDescriptorCountAllocInfo;

  // create descriptor set
  pEngineCore->AddObject(
      [this, &descriptorSetAllocateInfo]() {
        return pEngineCore->objCreate.VKAllocateDescriptorSet(
            &descriptorSetAllocateInfo, nullptr,
            &this->pipelineData.descriptorSet);
      },
      "compute_vertex_DescriptorSet");

  // storage input buffer descriptor info
  VkDescriptorBufferInfo modelVertexBufferDescriptor{};
  // modelVertexBufferDescriptor.buffer = modelVertexBuffer.bufferData.buffer;
  modelVertexBufferDescriptor.buffer = this->model->vertices.buffer;
  modelVertexBufferDescriptor.offset = 0;
  modelVertexBufferDescriptor.range =
      static_cast<uint32_t>(this->model->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage input buffer descriptor write info
  VkWriteDescriptorSet modelVertexBufferWrite{};
  modelVertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  modelVertexBufferWrite.dstSet = pipelineData.descriptorSet;
  modelVertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  modelVertexBufferWrite.dstBinding = 0;
  modelVertexBufferWrite.pBufferInfo = &modelVertexBufferDescriptor;
  modelVertexBufferWrite.descriptorCount = 1;

  std::cout << "this->model->vertexBufferSize: "
            << this->model->vertexBufferSize << std::endl;
  // storage output buffer descriptor info
  VkDescriptorBufferInfo vertexReadBufferDescriptor{};
  // vertexReadBufferDescriptor.buffer =
  // this->model->vertices.buffer.bufferData.buffer;
  vertexReadBufferDescriptor.buffer = vertexReadBuffer.bufferData.buffer;
  vertexReadBufferDescriptor.offset = 0;
  vertexReadBufferDescriptor.range =
      static_cast<uint32_t>(this->model->vertexCount) *
      sizeof(gtp::Model::Vertex);

  // storage output buffer descriptor write info
  VkWriteDescriptorSet vertexReadBufferWrite{};
  vertexReadBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  vertexReadBufferWrite.dstSet = pipelineData.descriptorSet;
  vertexReadBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  vertexReadBufferWrite.dstBinding = 1;
  vertexReadBufferWrite.pBufferInfo = &vertexReadBufferDescriptor;
  vertexReadBufferWrite.descriptorCount = 1;

  // transforms buffer descriptor info
  VkDescriptorBufferInfo transformMatrixBufferDescriptor{};
  transformMatrixBufferDescriptor.buffer =
      transformMatrixBuffer.bufferData.buffer;
  transformMatrixBufferDescriptor.offset = 0;
  transformMatrixBufferDescriptor.range = transformMatrixBuffer.bufferData.size;

  // transforms buffer descriptor write info
  VkWriteDescriptorSet transformMatrixBufferWrite{};
  transformMatrixBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  transformMatrixBufferWrite.dstSet = pipelineData.descriptorSet;
  transformMatrixBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  transformMatrixBufferWrite.dstBinding = 2;
  transformMatrixBufferWrite.pBufferInfo = &transformMatrixBufferDescriptor;
  transformMatrixBufferWrite.descriptorCount = 1;

  // geometry descriptor info
  VkDescriptorBufferInfo geometryDescriptor{};
  geometryDescriptor.buffer = geometryBuffer.bufferData.buffer;
  geometryDescriptor.offset = 0;
  geometryDescriptor.range = geometryBuffer.bufferData.size;

  // geometry descriptor write info
  VkWriteDescriptorSet geometryBufferWrite{};
  geometryBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  geometryBufferWrite.dstSet = pipelineData.descriptorSet;
  geometryBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  geometryBufferWrite.dstBinding = 3;
  geometryBufferWrite.pBufferInfo = &geometryDescriptor;
  geometryBufferWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      modelVertexBufferWrite, vertexReadBufferWrite, transformMatrixBufferWrite,
      geometryBufferWrite};

  // Image descriptors for the image array
  std::vector<VkDescriptorImageInfo> textureDescriptors{};

  for (int i = 0; i < this->model->textures.size(); i++) {
    VkDescriptorImageInfo descriptor{};
    descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor.sampler = this->model->textures[i].sampler;
    descriptor.imageView = this->model->textures[i].view;
    textureDescriptors.push_back(descriptor);
  }

  if (!this->model->textures.empty()) {
    VkWriteDescriptorSet writeDescriptorImgArray{};
    writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorImgArray.dstBinding = 4;
    writeDescriptorImgArray.descriptorType =
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writeDescriptorImgArray.descriptorCount =
        static_cast<uint32_t>(textureDescriptors.size());
    writeDescriptorImgArray.dstSet = this->pipelineData.descriptorSet;
    writeDescriptorImgArray.pImageInfo = textureDescriptors.data();
    writeDescriptorSets.push_back(writeDescriptorImgArray);
  }

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
                (static_cast<uint32_t>(this->model->vertexCount) / 256) + 1, 1,
                1);

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
// this->modelVertexBuffer.bufferData.memory,
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
// this->modelVertexBuffer.bufferData.memory);
//	//
//	//return tempVertex;
//
// }

void ComputeVertex::UpdateBoneBuffer() {

  std::vector<glm::mat4> boneMatrices;

  for (auto &nodes : this->model->linearNodes) {
    if (nodes->mesh) {
      if (nodes->skin) {
        for (int i = 0; i < nodes->skin->joints.size(); i++) {
          // std::cout << "node names: " << nodes->skin->joints[i]->name <<
          // std::endl;
          boneMatrices.push_back(nodes->mesh->uniformBlock.jointMatrix[i]);
        }
      }
    }
  }

  size_t boneBufferSize = boneMatrices.size() * sizeof(glm::mat4);

  this->boneBuffer.copyTo(boneMatrices.data(), boneBufferSize);
}

void ComputeVertex::UpdateTransformMatrixBuffer(
    Utilities_UI::TransformMatrices *pTransformMatrices) {
  // this->transformMatrices = *pTransformMatrices;
  this->transformMatrixBuffer.copyTo(pTransformMatrices,
                                     sizeof(Utilities_UI::TransformMatrices));
}

gtp::Buffer *ComputeVertex::GetBoneBuffer() { return &this->boneBuffer; }
