#include "Particle.hpp"

gtp::Particle::Particle() {}

gtp::Particle::Particle(EngineCore *corePtr) : Sphere(corePtr) {
  this->InitParticle(corePtr);
}

void gtp::Particle::DestroyParticle() {

  // descriptor pool
  // vkDestroyDescriptorPool(this->pEngineCore->devices.logical,
  //  this->pipelineData.descriptorPool, nullptr);

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

  // buffers
  this->buffers.computeBlockSSBO.destroy(this->pEngineCore->devices.logical);
  this->buffers.vertexSSBO.destroy(this->pEngineCore->devices.logical);
  this->buffers.vertexSSBOStaging.destroy(this->pEngineCore->devices.logical);

  this->DestroySphere();

  std::cout << " destroyed particle!" << std::endl;
}

void gtp::Particle::InitParticle(EngineCore *corePtr) {

  // -- initialize core pointer
  this->pEngineCore = corePtr;

  // -- initialize shader
  this->shader = gtp::Shader(pEngineCore);

  // -- create buffer particle storage buffer
  this->CreateVertexStorageBuffer();

  // -- create buffer particle storage buffer
  this->CreateComputeBlockStorageBuffer();

  // -- create particle compute pipeline
  this->CreateComputePipeline();

  std::cout << " initialized particle!" << std::endl;
}

void gtp::Particle::CreateVertexStorageBuffer() {

  VkDeviceSize storageBufferSize =
      this->SphereVertices().size() * sizeof(gtp::Model::Vertex);

  /* SSBO Staging Buffer */
  // -- init ssbo staging buffer and fill with particle attribute data
  this->buffers.vertexSSBOStaging.bufferData.bufferName =
      "gtp::Particle_ssbo_staging_buffer";
  this->buffers.vertexSSBOStaging.bufferData.bufferMemoryName =
      "gtp::Particle_ssbo_staging_bufferMemory";

  // -- create ssbo staging buffer
  this->pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
          VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &this->buffers.vertexSSBOStaging, storageBufferSize,
      this->SphereVertices().data());

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

void gtp::Particle::CreateParticleBLAS(
    std::vector<Utilities_AS::GeometryNode> *geometryNodeBuf,
    std::vector<Utilities_AS::GeometryIndex> *geometryIndexBuf,
    uint32_t textureOffset) {

  // Utilities_AS::GeometryIndex geometryIndex{};
  //
  // geometryIndex.nodeOffset = static_cast<int>(geometryNodeBuf->size());
  //
  // geometryIndexBuf->push_back(geometryIndex);
  //
  // for (auto& node : model->linearNodes) {
  //   if (node->mesh) {
  //
  //     for (auto& primitive : node->mesh->primitives) {
  //       if (primitive->indexCount > 0) {
  //
  //         VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress;
  //         VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress;
  //
  //         vertexBufferDeviceAddress.deviceAddress =
  //           Utilities_AS::getBufferDeviceAddress(pEngineCore,
  //             model->vertices.buffer);
  //
  //         indexBufferDeviceAddress.deviceAddress =
  //           Utilities_AS::getBufferDeviceAddress(pEngineCore,
  //             model->indices.buffer) +
  //           primitive->firstIndex * sizeof(uint32_t);
  //
  //         VkAccelerationStructureGeometryKHR geometry{};
  //         geometry.sType =
  //           VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  //         geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  //         geometry.geometry.triangles.sType =
  //           VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
  //         geometry.geometry.triangles.vertexFormat =
  //         VK_FORMAT_R32G32B32_SFLOAT; geometry.geometry.triangles.vertexData
  //         = vertexBufferDeviceAddress; geometry.geometry.triangles.maxVertex
  //         =
  //           static_cast<uint32_t>(model->vertexCount);
  //         geometry.geometry.triangles.vertexStride =
  //         sizeof(gtp::Model::Vertex); geometry.geometry.triangles.indexType =
  //         VK_INDEX_TYPE_UINT32; geometry.geometry.triangles.indexData =
  //         indexBufferDeviceAddress; blasData->geometries.push_back(geometry);
  //         blasData->maxPrimitiveCounts.push_back(primitive->indexCount / 3);
  //         blasData->maxPrimitiveCount += primitive->indexCount / 3;
  //
  //         VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
  //         buildRangeInfo.firstVertex = 0;
  //         buildRangeInfo.primitiveOffset = 0;
  //         buildRangeInfo.primitiveCount = primitive->indexCount / 3;
  //         buildRangeInfo.transformOffset = 0;
  //         blasData->buildRangeInfos.push_back(buildRangeInfo);
  //
  //         Utilities_AS::GeometryNode geometryNode{};
  //         geometryNode.vertexBufferDeviceAddress =
  //           vertexBufferDeviceAddress.deviceAddress;
  //         geometryNode.indexBufferDeviceAddress =
  //           indexBufferDeviceAddress.deviceAddress;
  //
  //         geometryNode.textureIndexBaseColor =
  //           primitive->material.baseColorTexture
  //           ? static_cast<int>(
  //             primitive->material.baseColorTexture->index +
  //             textureOffset)
  //           : -1;
  //
  //         geometryNode.textureIndexOcclusion =
  //           primitive->material.occlusionTexture
  //           ? static_cast<int>(
  //             primitive->material.occlusionTexture->index +
  //             textureOffset)
  //           : -1;
  //
  //         geometryNode.textureIndexMetallicRoughness =
  //           primitive->material.metallicRoughnessTexture
  //           ? static_cast<int>(
  //             primitive->material.metallicRoughnessTexture->index +
  //             textureOffset)
  //           : -1;
  //
  //         geometryNode.textureIndexNormal =
  //           primitive->material.normalTexture
  //           ? static_cast<int>(primitive->material.normalTexture->index +
  //             textureOffset)
  //           : -1;
  //
  //         geometryNode.semiTransparentFlag = model->semiTransparentFlag;
  //
  //         geometryNode.objectIDColor =
  //           (static_cast<float>(textureOffset) * 8.0f / 255.0f);
  //
  //         std::cout << "\ngeometry node obj id color: "
  //           << ((static_cast<float>(textureOffset) + 8.0f) / 255.0f)
  //           << std::endl;
  //
  //         geometryNodeBuf->push_back(geometryNode);
  //       }
  //     }
  //   }
  // }
  //
  // for (auto& rangeInfo : blasData->buildRangeInfos) {
  //   blasData->pBuildRangeInfos.push_back(&rangeInfo);
  // }
  //
  //// Get size info
  //// acceleration structure build geometry info
  // blasData->accelerationStructureBuildGeometryInfo.sType =
  //   VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  // blasData->accelerationStructureBuildGeometryInfo.type =
  //   VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  // blasData->accelerationStructureBuildGeometryInfo.flags =
  //   VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  // blasData->accelerationStructureBuildGeometryInfo.geometryCount =
  //   static_cast<uint32_t>(blasData->geometries.size());
  // blasData->accelerationStructureBuildGeometryInfo.pGeometries =
  //   blasData->geometries.data();
  //
  // blasData->accelerationStructureBuildSizesInfo.sType =
  //   VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  // pEngineCore->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
  //   pEngineCore->devices.logical,
  //   VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
  //   &blasData->accelerationStructureBuildGeometryInfo,
  //   blasData->maxPrimitiveCounts.data(),
  //   &blasData->accelerationStructureBuildSizesInfo);
  //
  // Utilities_AS::createAccelerationStructureBuffer(
  //   pEngineCore, &BLAS->memory, &BLAS->buffer,
  //   &blasData->accelerationStructureBuildSizesInfo,
  //   "Utilities_AS::createBLAS___AccelerationStructureBuffer");
  //
  // VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
  // accelerationStructureCreateInfo.sType =
  //   VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  // accelerationStructureCreateInfo.buffer = BLAS->buffer;
  // accelerationStructureCreateInfo.size =
  //   blasData->accelerationStructureBuildSizesInfo.accelerationStructureSize;
  // accelerationStructureCreateInfo.type =
  //   VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  //
  //// create acceleration structure
  // pEngineCore->add(
  //   [pEngineCore, &BLAS, &accelerationStructureCreateInfo]() {
  //     return pEngineCore->objCreate.VKCreateAccelerationStructureKHR(
  //       &accelerationStructureCreateInfo, nullptr,
  //       &BLAS->accelerationStructureKHR);
  //   },
  //   "Utilities_AS::createBLAS___BLASAccelerationStructureKHR");
  //
  //// create scratch buffer for acceleration structure build
  //// gtp::Buffer scratchBuffer;
  // Utilities_AS::createScratchBuffer(
  //   pEngineCore, &BLAS->scratchBuffer,
  //   blasData->accelerationStructureBuildSizesInfo.buildScratchSize,
  //   "Utilities_AS::createBLAS___blas_ScratchBufferBLAS");
  //
  // blasData->accelerationStructureBuildGeometryInfo.mode =
  //   VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  // blasData->accelerationStructureBuildGeometryInfo.dstAccelerationStructure =
  //   BLAS->accelerationStructureKHR;
  // blasData->accelerationStructureBuildGeometryInfo.scratchData.deviceAddress
  // =
  //   BLAS->scratchBuffer.bufferData.bufferDeviceAddress.deviceAddress;
  //
  // const VkAccelerationStructureBuildRangeInfoKHR* buildOffsetInfo =
  //   blasData->buildRangeInfos.data();
  //
  //// Build the acceleration structure on the device via a one-time command
  //// buffer submission create command buffer
  // VkCommandBuffer commandBuffer =
  // pEngineCore->objCreate.VKCreateCommandBuffer(
  //   VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  //
  //// build BLAS
  // pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
  //   commandBuffer, 1, &blasData->accelerationStructureBuildGeometryInfo,
  //   blasData->pBuildRangeInfos.data());
  //
  //// end and submit and destroy command buffer
  // pEngineCore->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
  //   pEngineCore->commandPools.graphics, true);
  //
  //// get acceleration structure device address
  // VkAccelerationStructureDeviceAddressInfoKHR
  // accelerationDeviceAddressInfo{}; accelerationDeviceAddressInfo.sType =
  //   VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  // accelerationDeviceAddressInfo.accelerationStructure =
  //   BLAS->accelerationStructureKHR;
  // BLAS->deviceAddress =
  //   pEngineCore->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(
  //     pEngineCore->devices.logical, &accelerationDeviceAddressInfo);
}
