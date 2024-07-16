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
  std::cout << " acceleration classes added bottomLevelASData to list"
            << std::endl;

  this->bottomLevelAccelerationStructures.push_back(bottomLevelASData);
}

void gtp::AccelerationStructures::CreateGeometryNodesBuffer() {
  buffers.geometry_nodes_buffer.bufferData.bufferName = "geometry_nodes_buffer";
  buffers.geometry_nodes_buffer.bufferData.bufferMemoryName =
      "geometry_nodes_bufferMemory";

  void* nodeData = nullptr;
  VkDeviceSize nodeBufSize = 0;

  if (!geometryNodes.empty()) {
    nodeData = geometryNodes.data();
    nodeBufSize =
        static_cast<uint32_t>(geometryNodes.size()) * sizeof(GeometryNode);
  }

  else {
    nodeData = new GeometryNode();
    nodeBufSize = sizeof(GeometryNode);
  }

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.geometry_nodes_buffer, nodeBufSize,
                                nodeData) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create geometry_nodes_buffer");
  }

  buffers.geometry_nodes_indices.bufferData.bufferName =
      "geometry_nodes_indicesBuffer";
  buffers.geometry_nodes_indices.bufferData.bufferMemoryName =
      "geometry_nodes_indicesBufferMemory";

  void* gIndexData = nullptr;
  VkDeviceSize gIndexBufSize = 0;

  if (!geometryNodesIndex.empty()) {
    gIndexData = geometryNodesIndex.data();
    gIndexBufSize =
        static_cast<uint32_t>(geometryNodesIndex.size()) * sizeof(int);
  }

  else {
    gIndexData = std::vector<int>(0).data();
    gIndexBufSize = sizeof(int);
  }

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.geometry_nodes_indices, gIndexBufSize,
                                gIndexData) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create geometry_nodes_index buffer");
  }
}

void gtp::AccelerationStructures::CreateTopLevelAccelerationStructure(
    std::vector<gtp::Model*> pModelList, Utilities_UI::ModelData modelData,
    std::vector<gtp::Particle*> pParticleList) {
  // blas instances buffer size decl.
  VkDeviceSize blasInstancesBufSize = 0;

  // void pointer for blas instances data
  void* blasInstancesData = nullptr;

  VkAccelerationStructureInstanceKHR tempASInstance{};

  /*OLD DELETE*/
  // resize renderer's blas instances 'buffer'
  // blasInstances.resize(this->bottomLevelAccelerationStructures.size());

  // if blas instances 'buffer' contains at least one element
  if (this->bottomLevelAccelerationStructures.size() != 0) {
    // iterate through the models list
    for (int i = 0; i < pModelList.size(); i++) {
      // handle the instances for particles if the model was loaded for use as a
      // particle
      if (i > 0 && pModelList[i]->isParticle) {
        for (int j = 0; j < PARTICLE_COUNT; j++) {
          VkAccelerationStructureInstanceKHR particleInstance;

          // Assign this VkTransformMatrixKHR to your instance
          particleInstance.transform =
              pParticleList[i]->ParticleTorusTransforms(i, modelData);
          particleInstance.instanceCustomIndex = i;
          particleInstance.mask = 0xFF;
          particleInstance.instanceShaderBindingTableRecordOffset = 0;
          particleInstance.accelerationStructureReference =
              this->bottomLevelAccelerationStructures[i]
                  ->accelerationStructure.deviceAddress;
          // std::cout << "acceleration_structures_class_.deviceAddress"
          //   << this->bottomLevelAccelerationStructures[particleIdx]
          //   ->accelerationStructure.deviceAddress
          //   << std::endl;
          this->topLevelAccelerationStructureData
              .bottomLevelAccelerationStructureInstances.push_back(
                  particleInstance);
        }
      }

      // otherwise handle each instance
      else {
        /* transform matrices */
        // rotation matrix
        glm::mat4 rotationMatrix = modelData.transformMatrices[i].rotate;

        // translation matrix
        glm::mat4 translationMatrix = modelData.transformMatrices[i].translate;

        // scale matrix
        glm::mat4 scaleMatrix = modelData.transformMatrices[i].scale;

        // Combine rotation, translation, and scale into a single 4x4 matrix
        glm::mat4 transformMatrix =
            translationMatrix * rotationMatrix * scaleMatrix;

        // Convert glm::mat4 to VkTransformMatrixKHR
        VkTransformMatrixKHR vkTransformMatrix{};

        for (int col = 0; col < 4; ++col) {
          for (int row = 0; row < 3; ++row) {
            vkTransformMatrix.matrix[row][col] =
                transformMatrix[col][row];  // Vulkan expects column-major order
          }
        }

        // assign VkTransformMatrixKHR to instance and initialize instance
        // struct data
        tempASInstance.transform = vkTransformMatrix;
        tempASInstance.instanceCustomIndex = i;
        tempASInstance.mask = 0xFF;
        tempASInstance.instanceShaderBindingTableRecordOffset = 0;
        // std::cout << "accel tlas test: " << i << std::endl;
        tempASInstance.accelerationStructureReference =
            this->bottomLevelAccelerationStructures[i]
                ->accelerationStructure.deviceAddress;
        // std::cout
        //     << "acceleration_structures_class_.bottom_level_AS_DeviceAddress"
        //     << this->bottomLevelAccelerationStructures[i]
        //            ->accelerationStructure.deviceAddress
        //     << std::endl;
        this->topLevelAccelerationStructureData
            .bottomLevelAccelerationStructureInstances.push_back(
                tempASInstance);
      }
      // std::cout << "outerTest: " << i << std::endl;
    }
  }

  /* instances buffer */
  // name buffer for debug hashmap
  buffers.tlas_instancesBuffer.bufferData.bufferName =
      "acceleration_structures_class_topLevelAS_InstancesBuffer";
  buffers.tlas_instancesBuffer.bufferData.bufferMemoryName =
      "acceleration_structures_class_topLevelAS_InstancesBufferMemory";

  // if blas instances 'buffer' has any elements
  if (this->bottomLevelAccelerationStructures.size() != 0) {
    // size of instance struct * number of instances in the 'buffer'
    blasInstancesBufSize =
        sizeof(VkAccelerationStructureInstanceKHR) *
        static_cast<uint32_t>(this->bottomLevelAccelerationStructures.size());

    // void pointer now points to instances 'buffer' data
    blasInstancesData = this->topLevelAccelerationStructureData
                            .bottomLevelAccelerationStructureInstances.data();
  }

  // if instances 'buffer' is empty, assign a single uninitialized AS instance
  // struct so buffer size is not zero and can be read/written to
  else {
    blasInstancesBufSize = sizeof(VkAccelerationStructureInstanceKHR);
  }

  // create instances buffer
  if (pEngineCore->CreateBuffer(
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &buffers.tlas_instancesBuffer, blasInstancesBufSize,
          blasInstancesData) != VK_SUCCESS) {
    throw std::invalid_argument(
        "failed to create accelerationStructuresClass instances buffer");
  }

  // get instance buffer device address
  topLevelAccelerationStructureData.instanceDataDeviceAddress.deviceAddress =
      pEngineCore->GetBufferDeviceAddress(
          buffers.tlas_instancesBuffer.bufferData.buffer);

  // initialize top level acceleration structure geometry struct
  topLevelAccelerationStructureData.accelerationStructureGeometry.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  topLevelAccelerationStructureData.accelerationStructureGeometry.geometryType =
      VK_GEOMETRY_TYPE_INSTANCES_KHR;
  topLevelAccelerationStructureData.accelerationStructureGeometry.flags =
      VK_GEOMETRY_OPAQUE_BIT_KHR;
  topLevelAccelerationStructureData.accelerationStructureGeometry.geometry
      .instances.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  topLevelAccelerationStructureData.accelerationStructureGeometry.geometry
      .instances.arrayOfPointers = VK_FALSE;
  topLevelAccelerationStructureData.accelerationStructureGeometry.geometry
      .instances.data =
      topLevelAccelerationStructureData.instanceDataDeviceAddress;

  /* get tlas size information */
  // Acceleration Structure Build Geometry Info
  topLevelAccelerationStructureData.accelerationStructureBuildGeometryInfo
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  topLevelAccelerationStructureData.accelerationStructureBuildGeometryInfo
      .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  topLevelAccelerationStructureData.accelerationStructureBuildGeometryInfo
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  topLevelAccelerationStructureData.accelerationStructureBuildGeometryInfo
      .geometryCount = 1;
  topLevelAccelerationStructureData.accelerationStructureBuildGeometryInfo
      .pGeometries =
      &topLevelAccelerationStructureData.accelerationStructureGeometry;

  // -- tlas data member
  // primitive count
  topLevelAccelerationStructureData.primitive_count = static_cast<uint32_t>(
      this->topLevelAccelerationStructureData
          .bottomLevelAccelerationStructureInstances.size());

  // -- acceleration Structure Build Sizes Info
  topLevelAccelerationStructureData.accelerationStructureBuildSizesInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  // -- get acceleration structure build sizes
  pEngineCore->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
      pEngineCore->devices.logical,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &topLevelAccelerationStructureData.accelerationStructureBuildGeometryInfo,
      &topLevelAccelerationStructureData.primitive_count,
      &topLevelAccelerationStructureData.accelerationStructureBuildSizesInfo);

  // -- create acceleration structure buffer
  this->CreateAccelerationStructureBuffer(
      this->topLevelAccelerationStructure,
      &topLevelAccelerationStructureData.accelerationStructureBuildSizesInfo,
      "accelerationStructures_topLevelAS_Buffer");

  // -- acceleration structure create info
  VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
  accelerationStructureCreateInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  accelerationStructureCreateInfo.buffer =
      this->topLevelAccelerationStructure.buffer;
  accelerationStructureCreateInfo.size =
      topLevelAccelerationStructureData.accelerationStructureBuildSizesInfo
          .accelerationStructureSize;
  accelerationStructureCreateInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  // -- create acceleration structure
  pEngineCore->add(
      [this, &accelerationStructureCreateInfo]() {
        return pEngineCore->objCreate.VKCreateAccelerationStructureKHR(
            &accelerationStructureCreateInfo, nullptr,
            &this->topLevelAccelerationStructure.accelerationStructureKHR);
      },
      "accelerationStructuresClass_accelerationStructureKHR");

  // -- create scratch buffer
  this->CreateScratchBuffer(
      buffers.tlas_scratch,
      topLevelAccelerationStructureData.accelerationStructureBuildSizesInfo
          .buildScratchSize,
      "acceleration_structures_class_ScratchBuffer_top_levelAS");

  // acceleration Build Geometry Info{};
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo.flags =
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo.mode =
      VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo
      .dstAccelerationStructure =
      this->topLevelAccelerationStructure.accelerationStructureKHR;
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo
      .geometryCount = 1;
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo.pGeometries =
      &topLevelAccelerationStructureData.accelerationStructureGeometry;
  topLevelAccelerationStructureData.accelerationBuildGeometryInfo.scratchData
      .deviceAddress =
      buffers.tlas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

  // acceleration structure build range info
  VkAccelerationStructureBuildRangeInfoKHR
      accelerationStructureBuildRangeInfo{};
  accelerationStructureBuildRangeInfo.primitiveCount =
      topLevelAccelerationStructureData.primitive_count;
  accelerationStructureBuildRangeInfo.primitiveOffset = 0;
  accelerationStructureBuildRangeInfo.firstVertex = 0;
  accelerationStructureBuildRangeInfo.transformOffset = 0;

  // array of acceleration structure build range info
  topLevelAccelerationStructureData.accelerationBuildStructureRangeInfos = {
      &accelerationStructureBuildRangeInfo};

  // build the acceleration structure on the device via a one-time command
  // buffer submission implementations can support acceleration structure
  // building on host
  // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands)
  // device builds are preferred

  // create one time submit command buffer
  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // build acceleration structure/s
  pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
      commandBuffer, 1,
      &topLevelAccelerationStructureData.accelerationBuildGeometryInfo,
      topLevelAccelerationStructureData.accelerationBuildStructureRangeInfos
          .data());

  // flush one time submit command buffer
  pEngineCore->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // get acceleration structure device address
  VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
  accelerationDeviceAddressInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  accelerationDeviceAddressInfo.accelerationStructure =
      this->topLevelAccelerationStructure.accelerationStructureKHR;

  // get and assign tlas acceleration structure device address value
  this->topLevelAccelerationStructure.deviceAddress =
      pEngineCore->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(
          pEngineCore->devices.logical, &accelerationDeviceAddressInfo);
}

void gtp::AccelerationStructures::InitAccelerationStructures(
    EngineCore* engineCorePtr) {
  // initialize core pointer
  this->pEngineCore = engineCorePtr;

  std::cout << "initialized acceleration structures class!" << std::endl;
}

void gtp::AccelerationStructures::BuildBottomLevelAccelerationStructure(
    gtp::Model* pModel) {
  this->CreateBottomLevelAccelerationStructure(pModel);
}

void gtp::AccelerationStructures::BuildTopLevelAccelerationStructure(
    std::vector<gtp::Model*> pModelList, Utilities_UI::ModelData modelData,
    std::vector<gtp::Particle*> pParticleList) {
  this->CreateTopLevelAccelerationStructure(pModelList, modelData,
                                            pParticleList);
}

void gtp::AccelerationStructures::BuildGeometryNodesBuffer() {
  this->CreateGeometryNodesBuffer();
}

gtp::AccelerationStructures::AccelerationStructures() {}

gtp::AccelerationStructures::AccelerationStructures(EngineCore* engineCorePtr) {
  // call init function
  this->InitAccelerationStructures(engineCorePtr);
}

void gtp::AccelerationStructures::DestroyAccelerationStructures() {
  // g node buffer
  this->buffers.geometry_nodes_buffer.destroy(
      this->pEngineCore->devices.logical);

  // g node indices buffer
  this->buffers.geometry_nodes_indices.destroy(
      this->pEngineCore->devices.logical);

  // -- bottom level acceleration structure & related buffers -- //
  for (int i = 0; i < this->bottomLevelAccelerationStructures.size(); i++) {
    // accel. structure
    pEngineCore->coreExtensions->vkDestroyAccelerationStructureKHR(
        pEngineCore->devices.logical,
        this->bottomLevelAccelerationStructures[i]
            ->accelerationStructure.accelerationStructureKHR,
        nullptr);

    // scratch buffer
    this->bottomLevelAccelerationStructures[i]
        ->accelerationStructure.scratchBuffer.destroy(
            this->pEngineCore->devices.logical);

    // accel structure buffer and memory
    vkDestroyBuffer(pEngineCore->devices.logical,
                    this->bottomLevelAccelerationStructures[i]
                        ->accelerationStructure.buffer,
                    nullptr);
    vkFreeMemory(pEngineCore->devices.logical,
                 this->bottomLevelAccelerationStructures[i]
                     ->accelerationStructure.memory,
                 nullptr);
  }

  // -- top level acceleration structure & related buffers -- //

  // accel. structure
  pEngineCore->coreExtensions->vkDestroyAccelerationStructureKHR(
      pEngineCore->devices.logical,
      this->topLevelAccelerationStructure.accelerationStructureKHR, nullptr);

  // scratch buffer
  buffers.tlas_scratch.destroy(this->pEngineCore->devices.logical);

  // instances buffer
  buffers.tlas_instancesBuffer.destroy(this->pEngineCore->devices.logical);

  // accel. structure buffer and memory
  vkDestroyBuffer(pEngineCore->devices.logical,
                  this->topLevelAccelerationStructure.buffer, nullptr);
  vkFreeMemory(pEngineCore->devices.logical,
               this->topLevelAccelerationStructure.memory, nullptr);

  // transforms buffer
  this->buffers.transformBuffer.destroy(this->pEngineCore->devices.logical);

  // output success
  std::cout << "successfully destroyed acceleration structures class"
            << std::endl;
}
