#include "gltf_viewer_rt_utils.hpp"

uint64_t gltf_viewer_rt_utils::getBufferDeviceAddress(CoreBase* pCoreBase, VkBuffer buffer) {
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return pCoreBase->coreExtensions->vkGetBufferDeviceAddressKHR(pCoreBase->devices.logical, &bufferDeviceAI);
}

void gltf_viewer_rt_utils::createScratchBuffer(CoreBase* pCoreBase, vrt::Buffer* buffer, VkDeviceSize size, std::string name) {
	//create buffer
	pCoreBase->add([pCoreBase, buffer, size]() {return buffer->CreateBuffer(pCoreBase->devices.physical,
		pCoreBase->devices.logical, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
		size, "scratchBuffer_");}, "scratchBuffer_" + name);

	//allocate memory
	pCoreBase->add([pCoreBase, buffer, name]() {return buffer->AllocateBufferMemory(pCoreBase->devices.physical,
		pCoreBase->devices.logical, "scratchBufferMemory_" + name);}, "scratchBufferMemory_" + name);

	//bind memory
	if (vkBindBufferMemory(pCoreBase->devices.logical, buffer->bufferData.buffer, buffer->bufferData.memory, 0)) {
		throw std::invalid_argument("Failed to bind " + name + " scratch buffer memory");
	}

	buffer->bufferData.bufferDeviceAddress.deviceAddress = gltf_viewer_rt_utils::getBufferDeviceAddress(pCoreBase, buffer->bufferData.buffer);

}

void gltf_viewer_rt_utils::createAccelerationStructureBuffer(CoreBase* coreBase, VkDeviceMemory* memory, VkBuffer* buffer,
	VkAccelerationStructureBuildSizesInfoKHR* buildSizeInfo, std::string bufferName) {

	//buffer create info
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buildSizeInfo->accelerationStructureSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	//create buffer
	coreBase->add([coreBase, &bufferCreateInfo, buffer]()
		{return
		coreBase->objCreate.VKCreateBuffer(&bufferCreateInfo, nullptr, buffer);}, bufferName);

	//memory requirements
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(coreBase->devices.logical, *buffer, &memoryRequirements);

	//allocate flags
	VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
	memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
	memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

	//allocate info
	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.pNext = &memoryAllocateFlagsInfo;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex =
		coreBase->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	std::string bufferNameWithSuffix = bufferName + "Memory";

	//create object/map handle
	coreBase->add([coreBase, &memoryAllocateInfo, memory]()
		{return
		coreBase->objCreate.VKAllocateMemory(&memoryAllocateInfo, nullptr, memory);},
		bufferNameWithSuffix);

	//bind memory to buffer
	if (vkBindBufferMemory(coreBase->devices.logical, *buffer, *memory, 0)
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to bind acceleration structure buffer memory ");
	}

}

void gltf_viewer_rt_utils::createStorageImage(CoreBase* pCoreBase, StorageImage* storageImage, std::string name) {

	//image create info
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = pCoreBase->swapchainData.assignedSwapchainImageFormat.format;
	imageCreateInfo.extent.width = pCoreBase->swapchainData.swapchainExtent2D.width;
	imageCreateInfo.extent.height = pCoreBase->swapchainData.swapchainExtent2D.height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//create image
	pCoreBase->add([pCoreBase, &imageCreateInfo, &storageImage]()
		{return pCoreBase->objCreate.VKCreateImage(&imageCreateInfo, nullptr, &storageImage->image);}, name);

	//image memory requirements
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(pCoreBase->devices.logical, storageImage->image, &memReqs);

	//image memory allocation information
	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memReqs.size;
	memoryAllocateInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	pCoreBase->add([pCoreBase, &memoryAllocateInfo, &storageImage]()
		{return pCoreBase->objCreate.VKAllocateMemory(&memoryAllocateInfo, nullptr, &storageImage->memory);},
		name + "_memory");

	//bind image memory
	if (vkBindImageMemory(pCoreBase->devices.logical, storageImage->image, storageImage->memory, 0) != VK_SUCCESS) {
		throw std::invalid_argument("failed to bind image memory");
	}

	//image view create info
	VkImageViewCreateInfo imageViewCreateInfo{};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	imageViewCreateInfo.format = pCoreBase->swapchainData.assignedSwapchainImageFormat.format;
	imageViewCreateInfo.subresourceRange = {};
	imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
	imageViewCreateInfo.subresourceRange.levelCount = 1;
	imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
	imageViewCreateInfo.subresourceRange.layerCount = 1;
	imageViewCreateInfo.image = storageImage->image;

	//create image view and map name/handle for debug
	pCoreBase->add([pCoreBase, &imageViewCreateInfo, &storageImage]()
		{return pCoreBase->objCreate.VKCreateImageView(&imageViewCreateInfo, nullptr, &storageImage->view);},
		name + "view");

	//create temporary command buffer
	VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//transition image layout
	vrt::Tools::setImageLayout(cmdBuffer, storageImage->image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	//submit temporary command buffer
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

}

void gltf_viewer_rt_utils::createBLAS(CoreBase* pCoreBase,
	std::vector<gltf_viewer_rt_utils::GeometryNode>* geometryNodeBuf,
	std::vector<gltf_viewer_rt_utils::GeometryIndex>* geometryIndexBuf,
	gltf_viewer_rt_utils::BLASData* blasData,
	gltf_viewer_rt_utils::AccelerationStructure* BLAS,
	GVM::Model* model,
	uint32_t textureOffset) {

	gltf_viewer_rt_utils::GeometryIndex geometryIndex{};

	geometryIndex.nodeOffset = static_cast<int>(geometryNodeBuf->size());

	geometryIndexBuf->push_back(geometryIndex);

	for (auto& node : model->linearNodes) {
		if (node->mesh) {

			//std::cout << "\n\n\nmulti_blas staticModel blas\n\n\n\n" << std::endl;

			for (auto& primitive : node->mesh->primitives) {
				if (primitive->indexCount > 0) {

					VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress;
					VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress;

					vertexBufferDeviceAddress.deviceAddress =
						gltf_viewer_rt_utils::getBufferDeviceAddress(pCoreBase,
							model->vertices.buffer);// +primitive.firstVertex * sizeof(GVM::Vertex);

					indexBufferDeviceAddress.deviceAddress =
						gltf_viewer_rt_utils::getBufferDeviceAddress(pCoreBase,
							model->indices.buffer) + primitive->firstIndex * sizeof(uint32_t);

					VkAccelerationStructureGeometryKHR geometry{};
					geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
					geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
					geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(model->vertexCount);
					//std::cout << "model->vertexCount: " << model->vertexCount << std::endl;
					geometry.geometry.triangles.vertexStride = sizeof(GVM::Model::Vertex);
					geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
					geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
					blasData->geometries.push_back(geometry);
					blasData->maxPrimitiveCounts.push_back(primitive->indexCount / 3);
					blasData->maxPrimitiveCount += primitive->indexCount / 3;

					VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
					buildRangeInfo.firstVertex = 0;
					buildRangeInfo.primitiveOffset = 0; // primitive.firstIndex * sizeof(uint32_t);
					buildRangeInfo.primitiveCount = primitive->indexCount / 3;
					//std::cout << "animated model buildRangeInfo.primitiveCount" << buildRangeInfo.primitiveCount << std::endl;
					buildRangeInfo.transformOffset = 0;
					blasData->buildRangeInfos.push_back(buildRangeInfo);

					gltf_viewer_rt_utils::GeometryNode geometryNode{};
					geometryNode.vertexBufferDeviceAddress = vertexBufferDeviceAddress.deviceAddress;
					geometryNode.indexBufferDeviceAddress = indexBufferDeviceAddress.deviceAddress;

					geometryNode.textureIndexBaseColor = primitive->material.baseColorTexture ? static_cast<int>(primitive->material.baseColorTexture->index + textureOffset) : -1;
					geometryNode.textureIndexOcclusion = primitive->material.occlusionTexture ? static_cast<int>(primitive->material.occlusionTexture->index + textureOffset) : -1;

					//geometryNode.textureIndexBaseColor = primitive->material.baseColorTexture ? static_cast<int>(primitive->material.texCoordSets.baseColor) + textureOffset : -1;

					//std::cout << "geometryNode.textureIndexBaseColor: " << geometryNode.textureIndexBaseColor << std::endl;

					//geometryNode.textureIndexOcclusion = primitive->material.occlusionTexture ? static_cast<int>(primitive->material.texCoordSets.occlusion) + textureOffset : -1;

					geometryNodeBuf->push_back(geometryNode);

				}
			}
		}
	}

	for (auto& rangeInfo : blasData->buildRangeInfos) {
		blasData->pBuildRangeInfos.push_back(&rangeInfo);
	}

	// Get size info
	//acceleration structure build geometry info
	blasData->accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	blasData->accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	blasData->accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
	blasData->accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(blasData->geometries.size());
	blasData->accelerationStructureBuildGeometryInfo.pGeometries = blasData->geometries.data();

	blasData->accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&blasData->accelerationStructureBuildGeometryInfo,
		blasData->maxPrimitiveCounts.data(),
		&blasData->accelerationStructureBuildSizesInfo);

	gltf_viewer_rt_utils::createAccelerationStructureBuffer(pCoreBase, &BLAS->memory, &BLAS->buffer,
		&blasData->accelerationStructureBuildSizesInfo, "gltf_viewer_rt_utils::createBLAS___AccelerationStructureBuffer");

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = BLAS->buffer;
	accelerationStructureCreateInfo.size = blasData->accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([pCoreBase, &BLAS, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&BLAS->accelerationStructureKHR);}, "gltf_viewer_rt_utils::createBLAS___BLASAccelerationStructureKHR");

	//create scratch buffer for acceleration structure build
	//vrt::Buffer scratchBuffer;
	gltf_viewer_rt_utils::createScratchBuffer(
		pCoreBase, &BLAS->scratchBuffer, blasData->accelerationStructureBuildSizesInfo.buildScratchSize, "gltf_viewer_rt_utils::createBLAS___blas_ScratchBufferBLAS");

	blasData->accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	blasData->accelerationStructureBuildGeometryInfo.dstAccelerationStructure = BLAS->accelerationStructureKHR;
	blasData->accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = BLAS->scratchBuffer.bufferData.bufferDeviceAddress.deviceAddress;

	const VkAccelerationStructureBuildRangeInfoKHR* buildOffsetInfo = blasData->buildRangeInfos.data();

	//Build the acceleration structure on the device via a one-time command buffer submission
	//create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//build BLAS
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&blasData->accelerationStructureBuildGeometryInfo,
		blasData->pBuildRangeInfos.data());

	//end and submit and destroy command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//get acceleration structure device address
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = BLAS->accelerationStructureKHR;
	BLAS->deviceAddress =
		pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
			&accelerationDeviceAddressInfo);
}

std::vector<GVM::Model::Vertex> gltf_viewer_rt_utils::GetVerticesFromBuffer(VkDevice device, GVM::Model* model) {

		void* data = nullptr;

		VkDeviceSize bufferSize = static_cast<uint32_t>(model->vertexCount) * sizeof(GVM::Model::Vertex);

		if (vkMapMemory(device, model->vertices.memory, 0, bufferSize, 0, &data) != VK_SUCCESS) {
			std::cerr << "\ngltf_viewer_rt_utils::GetVerticesFromBuffer failed to map memory\n";
		}
		
		std::vector<GVM::Model::Vertex> tempVertexBuffer{};
		tempVertexBuffer.resize(static_cast<uint32_t>(model->vertexCount));

		memcpy(tempVertexBuffer.data(), data, bufferSize);
		
		vkUnmapMemory(device, model->vertices.memory);
		
		return tempVertexBuffer;
	}
