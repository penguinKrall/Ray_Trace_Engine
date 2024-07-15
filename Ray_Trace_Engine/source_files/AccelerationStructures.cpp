#include "AccelerationStructures.hpp"

void gtp::AccelerationStructures::CreateScratchBuffer(gtp::Buffer& buffer,
                                                      VkDeviceSize size,
                                                      std::string name) {
  // create buffer
  pEngineCore->add(
      [this, &buffer, size]() {
        return buffer.CreateBuffer(
            pEngineCore->devices.physical, pEngineCore->devices.logical,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
            size, "scratchBuffer_");
      },
      "scratchBuffer_" + name);

  // allocate memory
  pEngineCore->add(
      [this, &buffer, name]() {
        return buffer.AllocateBufferMemory(pEngineCore->devices.physical,
                                           pEngineCore->devices.logical,
                                           "scratchBufferMemory_" + name);
      },
      "scratchBufferMemory_" + name);

  // bind memory
  if (vkBindBufferMemory(pEngineCore->devices.logical, buffer.bufferData.buffer,
                         buffer.bufferData.memory, 0)) {
    throw std::invalid_argument("Failed to bind " + name +
                                " scratch buffer memory");
  }

  buffer.bufferData.bufferDeviceAddress.deviceAddress =
      pEngineCore->GetBufferDeviceAddress(buffer.bufferData.buffer);
}

void gtp::AccelerationStructures::CreateAccelerationStructureBuffer(
    AccelerationStructure& accelerationStructure,
    VkAccelerationStructureBuildSizesInfoKHR* buildSizeInfo,
    std::string bufferName) {
  // buffer create info
  VkBufferCreateInfo bufferCreateInfo{};
  bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferCreateInfo.size = buildSizeInfo->accelerationStructureSize;
  bufferCreateInfo.usage =
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  // create buffer
  pEngineCore->add(
      [this, &bufferCreateInfo, &accelerationStructure]() {
        return pEngineCore->objCreate.VKCreateBuffer(
            &bufferCreateInfo, nullptr, &accelerationStructure.buffer);
      },
      bufferName);

  // memory requirements
  VkMemoryRequirements memoryRequirements{};
  vkGetBufferMemoryRequirements(pEngineCore->devices.logical,
                                accelerationStructure.buffer,
                                &memoryRequirements);

  // allocate flags
  VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
  memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
  memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

  // allocate info
  VkMemoryAllocateInfo memoryAllocateInfo{};
  memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
  memoryAllocateInfo.allocationSize = memoryRequirements.size;
  memoryAllocateInfo.memoryTypeIndex = pEngineCore->getMemoryType(
      memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  std::string bufferNameWithSuffix = bufferName + "Memory";

  // create object/map handle
  pEngineCore->add(
      [this, &memoryAllocateInfo, &accelerationStructure]() {
        return pEngineCore->objCreate.VKAllocateMemory(
            &memoryAllocateInfo, nullptr, &accelerationStructure.memory);
      },
      bufferNameWithSuffix);

  // bind memory to buffer
  if (vkBindBufferMemory(pEngineCore->devices.logical,
                         accelerationStructure.buffer,
                         accelerationStructure.memory, 0) != VK_SUCCESS) {
    throw std::invalid_argument(
        "failed to bind acceleration structure buffer memory ");
  }
}

void gtp::AccelerationStructures::CreateBottomLevelAccelerationStructure(
    gtp::Model* pModel) {
  // new blas instance
  BottomLevelAccelerationStructureData* bottomLevelASData =
      new BottomLevelAccelerationStructureData();

  // new geometry index
  GeometryIndex geometryIndex{};

  geometryIndex.nodeOffset = static_cast<int>(this->geometryNodes.size());

  geometryNodesIndex.push_back(geometryIndex);

  for (auto& node : pModel->linearNodes) {
    if (node->mesh) {
      for (auto& primitive : node->mesh->primitives) {
        if (primitive->indexCount > 0) {
          VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress;
          VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress;

          vertexBufferDeviceAddress.deviceAddress =
              pEngineCore->GetBufferDeviceAddress(pModel->vertices.buffer);

          indexBufferDeviceAddress.deviceAddress =
              pEngineCore->GetBufferDeviceAddress(pModel->indices.buffer) +
              primitive->firstIndex * sizeof(uint32_t);

          VkAccelerationStructureGeometryKHR geometry{};
          geometry.sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
          geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
          geometry.geometry.triangles.sType =
              VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
          geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
          geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
          geometry.geometry.triangles.maxVertex =
              static_cast<uint32_t>(pModel->vertexCount);
          geometry.geometry.triangles.vertexStride = sizeof(gtp::Model::Vertex);
          geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
          geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
          bottomLevelASData->geometries.push_back(geometry);
          bottomLevelASData->maxPrimitiveCounts.push_back(
              primitive->indexCount / 3);
          bottomLevelASData->maxPrimitiveCount += primitive->indexCount / 3;

          VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
          buildRangeInfo.firstVertex = 0;
          buildRangeInfo.primitiveOffset = 0;
          buildRangeInfo.primitiveCount = primitive->indexCount / 3;
          buildRangeInfo.transformOffset = 0;
          bottomLevelASData->buildRangeInfos.push_back(buildRangeInfo);

          GeometryNode geometryNode{};
          geometryNode.vertexBufferDeviceAddress =
              vertexBufferDeviceAddress.deviceAddress;
          geometryNode.indexBufferDeviceAddress =
              indexBufferDeviceAddress.deviceAddress;

          geometryNode.textureIndexBaseColor =
              primitive->material.baseColorTexture
                  ? static_cast<int>(
                        primitive->material.baseColorTexture->index +
                        textureOffset)
                  : -1;

          geometryNode.textureIndexOcclusion =
              primitive->material.occlusionTexture
                  ? static_cast<int>(
                        primitive->material.occlusionTexture->index +
                        textureOffset)
                  : -1;

          geometryNode.textureIndexMetallicRoughness =
              primitive->material.metallicRoughnessTexture
                  ? static_cast<int>(
                        primitive->material.metallicRoughnessTexture->index +
                        textureOffset)
                  : -1;

          geometryNode.textureIndexNormal =
              primitive->material.normalTexture
                  ? static_cast<int>(primitive->material.normalTexture->index +
                                     textureOffset)
                  : -1;

          geometryNode.semiTransparentFlag = pModel->semiTransparentFlag;

          geometryNode.objectIDColor =
              (static_cast<float>(textureOffset) * 8.0f / 255.0f);

          std::cout << "\ngeometry node obj id color: "
                    << ((static_cast<float>(textureOffset) + 8.0f) / 255.0f)
                    << std::endl;

          this->geometryNodes.push_back(geometryNode);
        }
      }
    }
  }

  for (auto& rangeInfo : bottomLevelASData->buildRangeInfos) {
    bottomLevelASData->pBuildRangeInfos.push_back(&rangeInfo);
  }

  // Get size info
  // acceleration structure build geometry info
  bottomLevelASData->accelerationStructureBuildGeometryInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  bottomLevelASData->accelerationStructureBuildGeometryInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  bottomLevelASData->accelerationStructureBuildGeometryInfo.flags =
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  bottomLevelASData->accelerationStructureBuildGeometryInfo.geometryCount =
      static_cast<uint32_t>(bottomLevelASData->geometries.size());
  bottomLevelASData->accelerationStructureBuildGeometryInfo.pGeometries =
      bottomLevelASData->geometries.data();

  bottomLevelASData->accelerationStructureBuildSizesInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
  pEngineCore->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
      pEngineCore->devices.logical,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &bottomLevelASData->accelerationStructureBuildGeometryInfo,
      bottomLevelASData->maxPrimitiveCounts.data(),
      &bottomLevelASData->accelerationStructureBuildSizesInfo);

  this->CreateAccelerationStructureBuffer(
      bottomLevelASData->accelerationStructure,
      &bottomLevelASData->accelerationStructureBuildSizesInfo,
      "createBLAS___AccelerationStructureBuffer");

  VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
  accelerationStructureCreateInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  accelerationStructureCreateInfo.buffer =
      bottomLevelASData->accelerationStructure.buffer;
  accelerationStructureCreateInfo.size =
      bottomLevelASData->accelerationStructureBuildSizesInfo
          .accelerationStructureSize;
  accelerationStructureCreateInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

  // create acceleration structure
  pEngineCore->add(
      [this, &bottomLevelASData, &accelerationStructureCreateInfo]() {
        return pEngineCore->objCreate.VKCreateAccelerationStructureKHR(
            &accelerationStructureCreateInfo, nullptr,
            &bottomLevelASData->accelerationStructure.accelerationStructureKHR);
      },
      "createBLAS___BLASAccelerationStructureKHR");

  // create scratch buffer for acceleration structure build
  // gtp::Buffer scratchBuffer;
  this->CreateScratchBuffer(
      bottomLevelASData->accelerationStructure.scratchBuffer,
      bottomLevelASData->accelerationStructureBuildSizesInfo.buildScratchSize,
      "createBLAS___blas_ScratchBufferBLAS");

  bottomLevelASData->accelerationStructureBuildGeometryInfo.mode =
      VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  bottomLevelASData->accelerationStructureBuildGeometryInfo
      .dstAccelerationStructure =
      bottomLevelASData->accelerationStructure.accelerationStructureKHR;
  bottomLevelASData->accelerationStructureBuildGeometryInfo.scratchData
      .deviceAddress = bottomLevelASData->accelerationStructure.scratchBuffer
                           .bufferData.bufferDeviceAddress.deviceAddress;

  const VkAccelerationStructureBuildRangeInfoKHR* buildOffsetInfo =
      bottomLevelASData->buildRangeInfos.data();

  // Build the acceleration structure on the device via a one-time command
  // buffer submission create command buffer
  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // build BLAS
  pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
      commandBuffer, 1,
      &bottomLevelASData->accelerationStructureBuildGeometryInfo,
      bottomLevelASData->pBuildRangeInfos.data());

  // end and submit and destroy command buffer
  pEngineCore->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // get acceleration structure device address
  VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
  accelerationDeviceAddressInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  accelerationDeviceAddressInfo.accelerationStructure =
      bottomLevelASData->accelerationStructure.accelerationStructureKHR;
  bottomLevelASData->accelerationStructure.deviceAddress =
      pEngineCore->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(
          pEngineCore->devices.logical, &accelerationDeviceAddressInfo);

  // increment texture offset by number of textures the model contains
  this->textureOffset += static_cast<uint32_t>(pModel->textures.size());

  // add bottom level acceleration structure data to vector of bottom level as
  // datas
  this->bottomLevelAccelerationStructures.push_back(bottomLevelASData);
}

void gtp::AccelerationStructures::InitAccelerationStructures(
    EngineCore* engineCorePtr) {
  // initialize core pointer
  this->pEngineCore = engineCorePtr;

  std::cout << "initialized acceleration structures class!" << std::endl;
}

gtp::AccelerationStructures::AccelerationStructures() {}

gtp::AccelerationStructures::AccelerationStructures(EngineCore* engineCorePtr) {
  // call init function
  this->InitAccelerationStructures(engineCorePtr);
}

void gtp::AccelerationStructures::DestroyAccelerationStructures() {
  // output success
  std::cout << "successfully destroyed acceleration structures class"
            << std::endl;
}
