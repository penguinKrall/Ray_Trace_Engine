#include "Raytracing.hpp"


void Raytracing::InitRaytracing(CoreBase* coreBase) {

	// init core pointer
	this->pCoreBase = coreBase;

	// init raytracing class instances shader class instance
	this->shader = Shader(coreBase);

	//load texture
	texture = vrt::Texture(this->pCoreBase);
	texture.loadFromFile("C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/textures/gratefloor_rgba.ktx",
		VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	//std::cout << "\nloaded vrt texture" << std::endl;

	// create bottom acceleration structure
	createBottomLevelAccelerationStructure();

	// create top acceleration structure
	createTopLevelAccelerationStructure();

	// create storage image
	createStorageImage();

	// create uniform buffer
	createUniformBuffer();

	// create raytracing pipeline
	createRayTracingPipeline();

	// create shader binding tables
	createShaderBindingTable();

	// create descriptor sets
	createDescriptorSets();

	//  build command buffers
	buildCommandBuffers();

}

Raytracing::Raytracing() {

}

Raytracing::Raytracing(CoreBase* coreBase) {
	// call this func to init core pointer
	InitRaytracing(coreBase);
}

void Raytracing::createScratchBuffer(vrt::Buffer* buffer, VkDeviceSize size, std::string name) {
	//create buffer
	pCoreBase->add([this, buffer, size]() {return buffer->CreateBuffer(this->pCoreBase->devices.physical,
		this->pCoreBase->devices.logical, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR,
		size, "scratchBuffer_");}, "scratchBuffer_" + name);

	//allocate memory
	pCoreBase->add([this, buffer, name]() {return buffer->AllocateBufferMemory(this->pCoreBase->devices.physical,
		this->pCoreBase->devices.logical, "scratchBufferMemory_" + name);}, "scratchBufferMemory_" + name);

	//bind memory
	if (vkBindBufferMemory(pCoreBase->devices.logical, buffer->bufferData.buffer, buffer->bufferData.memory, 0)) {
		throw std::invalid_argument("Failed to bind " + name + " scratch buffer memory");
	}

	buffer->bufferData.bufferDeviceAddress.deviceAddress = getBufferDeviceAddress(buffer->bufferData.buffer);

}

void Raytracing::deleteScratchBuffer(vrt::Buffer& buffer) {
	if (buffer.bufferData.memory != VK_NULL_HANDLE) {
		vkFreeMemory(pCoreBase->devices.logical, buffer.bufferData.memory, nullptr);
	}
	if (buffer.bufferData.buffer != VK_NULL_HANDLE) {
		vkDestroyBuffer(pCoreBase->devices.logical, buffer.bufferData.buffer, nullptr);
	}
}

void Raytracing::createAccelerationStructureBuffer(AccelerationStructure& accelerationStructure,
	VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, std::string bufferName) {

	//buffer create info
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = buildSizeInfo.accelerationStructureSize;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	//create buffer
	pCoreBase->add([this, &bufferCreateInfo, &accelerationStructure]()
		{return
		pCoreBase->objCreate.VKCreateBuffer(&bufferCreateInfo, nullptr, &accelerationStructure.buffer);}, bufferName);

	//memory requirements
	VkMemoryRequirements memoryRequirements{};
	vkGetBufferMemoryRequirements(pCoreBase->devices.logical, accelerationStructure.buffer, &memoryRequirements);

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
		pCoreBase->getMemoryType(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);


	std::string bufferNameWithSuffix = bufferName + "Memory";

	//create object/map handle
	pCoreBase->add([this, &memoryAllocateInfo, &accelerationStructure]()
		{return
		pCoreBase->objCreate.VKAllocateMemory(&memoryAllocateInfo, nullptr, &accelerationStructure.memory);},
		bufferNameWithSuffix);

	//bind memory to buffer
	if (vkBindBufferMemory(pCoreBase->devices.logical, accelerationStructure.buffer, accelerationStructure.memory, 0)
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to bind acceleration structure buffer memory ");
	}

}

void Raytracing::createBottomLevelAccelerationStructure() {
	//vertices for triangle
	// Setup vertices for a single triangle
	struct Vertex {
		float pos[3];
		float normal[3];
		float uv[2];
	};
	std::vector<Vertex> vertices = {
		{ {   0.5f,   0.5f, 0.0f }, {.0f, 0.0f, 1.0f}, { 1.0f, 1.0f} },
		{ {  -0.5f,   0.5f, 0.0f }, {.0f, 0.0f, 1.0f}, { 0.0f, 1.0f} },
		{ {  -0.5f,  -0.5f, 0.0f }, {.0f, 0.0f, 1.0f}, { 0.0f, 0.0f} },
		{ {   0.5f,  -0.5f, 0.0f }, {.0f, 0.0f, 1.0f}, { 1.0f, 0.0f} },
	};

	// Setup indices
	std::vector<uint32_t> indices = { 0, 1, 2, 0, 3, 2 };
	indexCount = static_cast<uint32_t>(indices.size());

	//identity transform matrix
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f
	};

	//create buffers for acceleration structure
	//
	//vertex buffer
	buffers.vertexBuffer.bufferData.bufferName = "raytracingVertexBuffer";
	buffers.vertexBuffer.bufferData.bufferMemoryName = "raytracingVertexBufferMemory";

	if (pCoreBase->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffers.vertexBuffer, vertices.size() * sizeof(Vertex), vertices.data())
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to create vertex buffer!");
	}

	//index buffer
	buffers.indexBuffer.bufferData.bufferName = "raytracingIndexBuffer";
	buffers.indexBuffer.bufferData.bufferMemoryName = "raytracingIndexBufferMemory";

	if (pCoreBase->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffers.indexBuffer, indices.size() * sizeof(uint32_t), indices.data())
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to create index buffer!");
	}

	//transform buffer
	buffers.transformBuffer.bufferData.bufferName = "raytracingTransformBuffer";
	buffers.transformBuffer.bufferData.bufferMemoryName = "raytracingTransformBufferMemory";

	if (pCoreBase->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		| VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
		| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &buffers.transformBuffer, sizeof(VkTransformMatrixKHR), &transformMatrix)
		!= VK_SUCCESS) {
		throw std::invalid_argument("failed to create tansform buffer!");
	}

	//buffer device addresses
	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

	vertexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(buffers.vertexBuffer.bufferData.buffer);

	indexBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(buffers.indexBuffer.bufferData.buffer);

	transformBufferDeviceAddress.deviceAddress = getBufferDeviceAddress(buffers.transformBuffer.bufferData.buffer);

	//acceleration structure data
	//geometry
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = 3;
	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(Vertex);
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;
	accelerationStructureGeometry.geometry.triangles.transformData = transformBufferDeviceAddress;

	//sizes info
	//geometry
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	//const uint32_t numTriangles = 1;
	const uint32_t numTriangles = static_cast<uint32_t>(indices.size() / 3);

	//build sizes
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};

	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	//create acceleration structure buffer
	createAccelerationStructureBuffer(bottomLevelAS, accelerationStructureBuildSizesInfo, "bottomLevelAS");

	//acceleration structure create info
	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo,
			nullptr, &bottomLevelAS.accelerationStructureKHR);}, "bottomLevelAS.accelerationStructureKHR");

	//create scratch buffer used during build of the bottom level acceleration structure
	vrt::RTObjects::createScratchBuffer(this->pCoreBase, &buffers.scratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize, "raytracingBOTAS");

	//geometry build into
	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.accelerationStructureKHR;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = buffers.scratchBuffer.bufferData.bufferDeviceAddress.deviceAddress;

	//build range info
	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = {
		&accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host
	//  (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	//create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//build acceleration structure on device
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());

	//flush command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.accelerationStructureKHR;
	bottomLevelAS.deviceAddress =
		pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
			&accelerationDeviceAddressInfo);

	deleteScratchBuffer(buffers.scratchBuffer);
}

void Raytracing::createTopLevelAccelerationStructure() {

	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };

	VkAccelerationStructureInstanceKHR instance{};
	instance.transform = transformMatrix;
	instance.instanceCustomIndex = 0;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	instance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
	instance.accelerationStructureReference = bottomLevelAS.deviceAddress;

	// Buffer for instance data
	/*vrt::Buffer instancesBuffer;*/

	//name
	buffers.instancesBuffer.bufferData.bufferName = "instancesBuffer";
	buffers.instancesBuffer.bufferData.bufferMemoryName = "instancesBufferMemory";


	//create buffer and allocate memory
	if (pCoreBase->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.instancesBuffer,
		sizeof(VkAccelerationStructureInstanceKHR), &instance) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create top level acceleration structure instances buffer");
	}

	//get instances buffer device address
	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	instanceDataDeviceAddress.deviceAddress = getBufferDeviceAddress(buffers.instancesBuffer.bufferData.buffer);

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	//get size info
	// 
	//	The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored.
	//  Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress member of
	//  VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.

	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	uint32_t primitive_count = 1;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;


	pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&primitive_count,
		&accelerationStructureBuildSizesInfo);

	createAccelerationStructureBuffer(topLevelAS, accelerationStructureBuildSizesInfo, "topLevelASBuffer");

	//acceleration structure create info
	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = topLevelAS.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&topLevelAS.accelerationStructureKHR);}, "topLevelAS.accelerationStructureKHR");

	// Create a small scratch buffer used during build of the top level acceleration structure
		//create scratch buffer used during build of the bottom level acceleration structure
	createScratchBuffer(&buffers.scratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize, "raytracingTOPAS");

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = topLevelAS.accelerationStructureKHR;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = buffers.scratchBuffer.bufferData.bufferDeviceAddress.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = 1;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = {
		&accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());

	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = topLevelAS.accelerationStructureKHR;

	topLevelAS.deviceAddress = pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
		&accelerationDeviceAddressInfo);

	deleteScratchBuffer(buffers.scratchBuffer);

	//buffers.instancesBuffer.destroy(pCoreBase->devices.logical);

}

void Raytracing::createStorageImage() {

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
	pCoreBase->add([this, &imageCreateInfo]()
		{return pCoreBase->objCreate.VKCreateImage(&imageCreateInfo, nullptr, &storageImage.image);}, "raytracingStorageImage");

	//image memory requirements
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(pCoreBase->devices.logical, storageImage.image, &memReqs);

	//image memory allocation information
	VkMemoryAllocateInfo memoryAllocateInfo{};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memReqs.size;
	memoryAllocateInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	pCoreBase->add([this, &memoryAllocateInfo]()
		{return pCoreBase->objCreate.VKAllocateMemory(&memoryAllocateInfo, nullptr, &storageImage.memory);},
		"raytracingStorageImageMemory");

	//bind image memory
	if (vkBindImageMemory(pCoreBase->devices.logical, storageImage.image, storageImage.memory, 0) != VK_SUCCESS) {
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
	imageViewCreateInfo.image = storageImage.image;

	//if (vkCreateImageView(pCoreBase->devices.logical, &imageViewCreateInfo, nullptr, &storageImage.view) != VK_SUCCESS) {
	//	throw std::invalid_argument("failed to create image view");
	//}

	//create image view and map name/handle for debug
	pCoreBase->add([this, &imageViewCreateInfo]()
		{return pCoreBase->objCreate.VKCreateImageView(&imageViewCreateInfo, nullptr, &storageImage.view);},
		"raytracingStorageImageView");

	//create temporary command buffer
	VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//transition image layout
	vrt::Tools::setImageLayout(cmdBuffer, storageImage.image,
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_GENERAL,
		{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

	//submit temporary command buffer
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

}

void Raytracing::updateUniformBuffers() {
	// Calculate uniform data
	uniformData.projInverse = glm::inverse(glm::perspective(glm::radians(pCoreBase->camera->Zoom),
		float(pCoreBase->swapchainData.swapchainExtent2D.width) / float(pCoreBase->swapchainData.swapchainExtent2D.height), 0.1f, 10.0f));

	uniformData.viewInverse = glm::inverse(pCoreBase->camera->GetViewMatrix());

	//buffer is mapped on creation
	//can be copied to now
	memcpy(buffers.ubo.bufferData.mapped, &uniformData, sizeof(uniformData));
}


void Raytracing::createUniformBuffer() {

	buffers.ubo.bufferData.bufferName = "uboBuffer";
	buffers.ubo.bufferData.bufferMemoryName = "uboBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.ubo, sizeof(uniformData), nullptr) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create uniform buffer!");
	}

	if (buffers.ubo.map(pCoreBase->devices.logical) != VK_SUCCESS) {
		throw std::invalid_argument("failed to map uniform buffer");
	}

	updateUniformBuffers();
}

void Raytracing::createRayTracingPipeline() {

	// acceleration structure binding
	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
	accelerationStructureLayoutBinding.binding = 0;
	accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
	accelerationStructureLayoutBinding.descriptorCount = 1;
	accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

	// ?? result image layout binding -- come back to this n delete when understand
	VkDescriptorSetLayoutBinding resultImageLayoutBinding{};
	resultImageLayoutBinding.binding = 1;
	resultImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resultImageLayoutBinding.descriptorCount = 1;
	resultImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

	// uniform buffer layout binding
	VkDescriptorSetLayoutBinding uniformBufferBinding{};
	uniformBufferBinding.binding = 2;
	uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferBinding.descriptorCount = 1;
	uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR |
		VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

	//texture layout binding
	VkDescriptorSetLayoutBinding textureLayoutBinding{};
	textureLayoutBinding.binding = 3;
	textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureLayoutBinding.descriptorCount = 1;
	textureLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;

	//bindings array
	std::vector<VkDescriptorSetLayoutBinding> bindings({
		accelerationStructureLayoutBinding,
		resultImageLayoutBinding,
		uniformBufferBinding,
		textureLayoutBinding
		});

	//descriptor set layout create info
	VkDescriptorSetLayoutCreateInfo descriptorSetlayoutCreateInfo{};
	descriptorSetlayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetlayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetlayoutCreateInfo.pBindings = bindings.data();

	//create descriptor set layout
	pCoreBase->add([this, &descriptorSetlayoutCreateInfo]()
		{return pCoreBase->objCreate.VKCreateDescriptorSetLayout(&descriptorSetlayoutCreateInfo,
			nullptr, &pipelineData.descriptorSetLayout);},
		"raytracingDescriptorSetLayout");

	// We pass buffer references for vertex and index buffers via push constants
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(uint64_t) * 2;

	//pipeline layout create info
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
	pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

	//create pipeline layout
	pCoreBase->add([this, &pipelineLayoutCreateInfo]()
		{return pCoreBase->objCreate.VKCreatePipelineLayout(&pipelineLayoutCreateInfo,
			nullptr, &pipelineData.pipelineLayout);}, "raytracingPipelineLayout");

	//project directory for loading shader modules
	std::filesystem::path projDirectory = std::filesystem::current_path();
	////std::cout << "Project Directory: " << projDirectory.string() << std::endl;

	// -- load raytracing shader groups
			// Shader Stages
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	// Ray Generation Shader Group
	{
		// Load ray generation shader module
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/raygen.rgen.spv",
			VK_SHADER_STAGE_RAYGEN_BIT_KHR, "rayGenShaderModule"));
		// Create shader group
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	// Miss Shader Group
	{
		// Load miss shader module
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/miss.rmiss.spv",
			VK_SHADER_STAGE_MISS_BIT_KHR, "rayMissShaderModule"));
		// Create shader group
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	// Closest Hit and Any Hit Shader Group
	{
		// Load closest hit shader module
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/closesthit.rchit.spv",
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "closetHitShaderModule"));
		// Load any hit shader module
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/anyhit.rahit.spv",
			VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "anyHitShaderModule"));
		// Create shader group
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 2; // Index of closest hit shader
		shaderGroup.anyHitShader = static_cast<uint32_t>(shaderStages.size()) - 1; // Index of any hit shader
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	//std::cout << "\nSHADER GROUP SIZE: " << shaderGroups.size() << std::endl;
	//std::cout << "\nSHADER STAGES SIZE: " << shaderStages.size() << std::endl;

	//pipeline create info
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
	rayTracingPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCreateInfo.pStages = shaderStages.data();
	rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCreateInfo.pGroups = shaderGroups.data();
	rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
	rayTracingPipelineCreateInfo.layout = pipelineData.pipelineLayout;
	

	//create raytracing pipeline
	pCoreBase->add([this, &rayTracingPipelineCreateInfo]()
		{return pCoreBase->objCreate.VKCreateRaytracingPipeline(&rayTracingPipelineCreateInfo,
			nullptr, &pipelineData.pipeline);}, "raytracingPipeline");
}

void Raytracing::createShaderBindingTable() {
	// handle size
	const uint32_t handleSize = pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize;

	// aligned handle size
	const uint32_t handleSizeAligned = vrt::Tools::alignedSize(
		pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
		pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleAlignment);

	// group count
	const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

	// total SBT size
	const uint32_t sbtSize = groupCount * handleSizeAligned;

	// shader handle storage
	std::vector<uint8_t> shaderHandleStorage(sbtSize);

	// get ray tracing shader handle group sizes
	if (pCoreBase->coreExtensions->vkGetRayTracingShaderGroupHandlesKHR(pCoreBase->devices.logical, pipelineData.pipeline,
		0, groupCount, sbtSize, shaderHandleStorage.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to get ray tracing shader group handle sizes");
	}

	// buffer usage flags
	const VkBufferUsageFlags bufferUsageFlags =
		VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

	// memory usage flags
	const VkMemoryPropertyFlags memoryUsageFlags =
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	try {
		// create buffers
		// raygen
		buffers.raygenShaderBindingTable.bufferData.bufferName = "raygenShaderBindingTable";
		buffers.raygenShaderBindingTable.bufferData.bufferMemoryName = "raygenShaderBindingTableMemory";
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &buffers.raygenShaderBindingTable, handleSize, nullptr)
			!= VK_SUCCESS) {
			throw std::runtime_error("failed to create raygenShaderBindingTable");
		}

		// miss
		buffers.missShaderBindingTable.bufferData.bufferName = "missShaderBindingTable";
		buffers.missShaderBindingTable.bufferData.bufferMemoryName = "missShaderBindingTableMemory";
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &buffers.missShaderBindingTable, handleSize, nullptr)
			!= VK_SUCCESS) {
			throw std::runtime_error("failed to create missShaderBindingTable");
		}

		// hit
		buffers.hitShaderBindingTable.bufferData.bufferName = "hitShaderBindingTable";
		buffers.hitShaderBindingTable.bufferData.bufferMemoryName = "hitShaderBindingTableMemory";
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &buffers.hitShaderBindingTable, handleSize * 2, nullptr)
			!= VK_SUCCESS) {
			throw std::runtime_error("failed to create hitShaderBindingTable");
		}

		// copy buffers
		buffers.raygenShaderBindingTable.map(pCoreBase->devices.logical);
		memcpy(buffers.raygenShaderBindingTable.bufferData.mapped, shaderHandleStorage.data(), handleSize);
		buffers.raygenShaderBindingTable.unmap(pCoreBase->devices.logical);

		buffers.missShaderBindingTable.map(pCoreBase->devices.logical);
		memcpy(buffers.missShaderBindingTable.bufferData.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize);
		buffers.missShaderBindingTable.unmap(pCoreBase->devices.logical);

		buffers.hitShaderBindingTable.map(pCoreBase->devices.logical);
		memcpy(buffers.hitShaderBindingTable.bufferData.mapped, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
		buffers.hitShaderBindingTable.unmap(pCoreBase->devices.logical);
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating shader binding table: " << e.what() << std::endl;
		// Handle error (e.g., cleanup resources, rethrow, etc.)
		// You may want to add additional error handling here based on your application's requirements
		throw;
	}
}


void Raytracing::createDescriptorSets() {

	//pool sizes
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = 1;

	//create descriptor pool
	pCoreBase->add([this, &descriptorPoolCreateInfo]()
		{return pCoreBase->objCreate.VKCreateDescriptorPool(&descriptorPoolCreateInfo,
			nullptr, &descriptorPool);}, "raytracingDescriptorPool");

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = &this->pipelineData.descriptorSetLayout;
	descriptorSetAllocateInfo.descriptorSetCount = 1;

	//create descriptor set
	pCoreBase->add([this, &descriptorSetAllocateInfo]()
		{return pCoreBase->objCreate.VKAllocateDescriptorSet(&descriptorSetAllocateInfo,
			nullptr, &this->pipelineData.descriptorSet);}, "raytracingDescriptorSet");

	//acceleration structure descriptor info
	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.accelerationStructureKHR;

	//acceleration structure descriptor write info
	VkWriteDescriptorSet accelerationStructureWrite{};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = pipelineData.descriptorSet;
	accelerationStructureWrite.dstBinding = 0;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	//storage image descriptor info
	VkDescriptorImageInfo storageImageDescriptor{};
	storageImageDescriptor.imageView = storageImage.view;
	storageImageDescriptor.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

	//storage/result image write
	VkWriteDescriptorSet resultImageWrite{};
	resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	resultImageWrite.dstSet = pipelineData.descriptorSet;
	resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resultImageWrite.dstBinding = 1;
	resultImageWrite.pImageInfo = &storageImageDescriptor;
	resultImageWrite.descriptorCount = 1;

	//ubo descriptor info
	VkDescriptorBufferInfo uboDescriptor{};
	uboDescriptor.buffer = buffers.ubo.bufferData.buffer;
	uboDescriptor.offset = 0;
	uboDescriptor.range = buffers.ubo.bufferData.size;

	//ubo descriptor write info
	VkWriteDescriptorSet uniformBufferWrite{};
	uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformBufferWrite.dstSet = pipelineData.descriptorSet;
	uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferWrite.dstBinding = 2;
	uniformBufferWrite.pBufferInfo = &uboDescriptor;
	uniformBufferWrite.descriptorCount = 1;

	//storage/result image write
	VkWriteDescriptorSet textureImageWrite{};
	textureImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	textureImageWrite.dstSet = pipelineData.descriptorSet;
	textureImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureImageWrite.dstBinding = 3;
	textureImageWrite.pImageInfo = &texture.descriptor;
	textureImageWrite.descriptorCount = 1;

	//array of descriptor write infos
	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		accelerationStructureWrite,
		resultImageWrite,
		uniformBufferWrite,
		textureImageWrite
	};

	//update descriptor set
	vkUpdateDescriptorSets(pCoreBase->devices.logical, static_cast<uint32_t>(writeDescriptorSets.size()),
		writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

}

void Raytracing::buildCommandBuffers() {
	//handle resize if window is resized
	//if (resized) {
	//	handleResize();
	//}

	VkCommandBufferBeginInfo cmdBufInfo{};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	for (int32_t i = 0; i < pCoreBase->commandBuffers.graphics.size(); ++i) {

		//std::cout << " command buffers [" << i << "]" << std::endl;

		if (vkBeginCommandBuffer(pCoreBase->commandBuffers.graphics[i], &cmdBufInfo) != VK_SUCCESS) {
			throw std::invalid_argument("failed to begin recording graphics command buffer");
		}

		//setup buffer regions pointing to shaders in shader binding table
		const uint32_t handleSizeAligned = vrt::Tools::alignedSize(
			pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
			pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleAlignment);

		//VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
		buffers.raygenStridedDeviceAddressRegion.deviceAddress = getBufferDeviceAddress(buffers.raygenShaderBindingTable.bufferData.buffer);
		buffers.raygenStridedDeviceAddressRegion.stride = handleSizeAligned;
		buffers.raygenStridedDeviceAddressRegion.size = handleSizeAligned;

		//VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
		buffers.missStridedDeviceAddressRegion.deviceAddress = getBufferDeviceAddress(buffers.missShaderBindingTable.bufferData.buffer);
		buffers.missStridedDeviceAddressRegion.stride = handleSizeAligned;
		buffers.missStridedDeviceAddressRegion.size = handleSizeAligned;

		//VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
		buffers.hitStridedDeviceAddressRegion.deviceAddress = getBufferDeviceAddress(buffers.hitShaderBindingTable.bufferData.buffer);
		buffers.hitStridedDeviceAddressRegion.stride = handleSizeAligned;
		buffers.hitStridedDeviceAddressRegion.size = handleSizeAligned;

		VkStridedDeviceAddressRegionKHR emptyShaderSbtEntry{};

		//dispatch the ray tracing commands
		vkCmdBindPipeline(pCoreBase->commandBuffers.graphics[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipeline);

		vkCmdBindDescriptorSets(pCoreBase->commandBuffers.graphics[i],
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipelineLayout, 0, 1, &pipelineData.descriptorSet, 0, nullptr);

		struct BufferReferences {
			uint64_t vertices;
			uint64_t indices;
		};

		BufferReferences bufferReferences{};

		bufferReferences.vertices = getBufferDeviceAddress(buffers.vertexBuffer.bufferData.buffer);
		bufferReferences.indices = getBufferDeviceAddress(buffers.indexBuffer.bufferData.buffer);

		// We set the buffer references for the mesh to be rendered using a push constant
		// If we wanted to render multiple objecets this would make it very easy to access their vertex and index buffers
		vkCmdPushConstants(pCoreBase->commandBuffers.graphics[i], this->pipelineData.pipelineLayout,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, 0, sizeof(uint64_t) * 2, &bufferReferences);

		pCoreBase->coreExtensions->vkCmdTraceRaysKHR(
			pCoreBase->commandBuffers.graphics[i],
			&buffers.raygenStridedDeviceAddressRegion,
			&buffers.missStridedDeviceAddressRegion,
			&buffers.hitStridedDeviceAddressRegion,
			&emptyShaderSbtEntry,
			pCoreBase->swapchainData.swapchainExtent2D.width,
			pCoreBase->swapchainData.swapchainExtent2D.height,
			1);

		//copy ray tracing output to swap chain image
		//prepare current swap chain image as transfer destination
		vrt::Tools::setImageLayout(
			pCoreBase->commandBuffers.graphics[i],
			pCoreBase->swapchainData.swapchainImages.image[i],
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			subresourceRange);

		//prepare ray tracing output image as transfer source
		vrt::Tools::setImageLayout(
			pCoreBase->commandBuffers.graphics[i],
			storageImage.image,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			subresourceRange);

		VkImageCopy copyRegion{};
		copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyRegion.srcOffset = { 0, 0, 0 };
		copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
		copyRegion.dstOffset = { 0, 0, 0 };
		copyRegion.extent = { pCoreBase->swapchainData.swapchainExtent2D.width,
			pCoreBase->swapchainData.swapchainExtent2D.height, 1 };

		vkCmdCopyImage(pCoreBase->commandBuffers.graphics[i], storageImage.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pCoreBase->swapchainData.swapchainImages.image[i],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		//transition swap chain image back for presentation
		vrt::Tools::setImageLayout(
			pCoreBase->commandBuffers.graphics[i],
			pCoreBase->swapchainData.swapchainImages.image[i],
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			subresourceRange);

		//transition ray tracing output image back to general layout
		vrt::Tools::setImageLayout(
			pCoreBase->commandBuffers.graphics[i],
			storageImage.image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_IMAGE_LAYOUT_GENERAL,
			subresourceRange);

		if (vkEndCommandBuffer(pCoreBase->commandBuffers.graphics[i]) != VK_SUCCESS) {
			throw std::invalid_argument("failed to end recording command buffer");
		}

	}
}

void Raytracing::rebuildCommandBuffers(uint32_t frame) {
	//handle resize if window is resized
	//if (resized) {
	//	handleResize();
	//}


		//subresource range
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	if (pCoreBase->camera->viewUpdated) {

		//setup buffer regions pointing to shaders in shader binding table
		const uint32_t handleSizeAligned = vrt::Tools::alignedSize(
			pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
			pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleAlignment);

		VkStridedDeviceAddressRegionKHR emptySbtEntry{};

		//dispatch the ray tracing commands
		vkCmdBindPipeline(pCoreBase->commandBuffers.graphics[frame], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipeline);

		vkCmdBindDescriptorSets(pCoreBase->commandBuffers.graphics[frame],
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipelineLayout, 0, 1, &pipelineData.descriptorSet, 0, nullptr);

		struct BufferReferences {
			uint64_t vertices;
			uint64_t indices;
		} ;

		BufferReferences bufferReferences{};

		bufferReferences.vertices = getBufferDeviceAddress(buffers.vertexBuffer.bufferData.buffer);
		bufferReferences.indices = getBufferDeviceAddress(buffers.indexBuffer.bufferData.buffer);

		// We set the buffer references for the mesh to be rendered using a push constant
		// If we wanted to render multiple objecets this would make it very easy to access their vertex and index buffers
		vkCmdPushConstants(pCoreBase->commandBuffers.graphics[frame], this->pipelineData.pipelineLayout,
			VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, 0, sizeof(uint64_t) * 2, &bufferReferences);

		pCoreBase->coreExtensions->vkCmdTraceRaysKHR(
			pCoreBase->commandBuffers.graphics[frame],
			&buffers.raygenStridedDeviceAddressRegion,
			&buffers.missStridedDeviceAddressRegion,
			&buffers.hitStridedDeviceAddressRegion,
			&emptySbtEntry,
			pCoreBase->swapchainData.swapchainExtent2D.width,
			pCoreBase->swapchainData.swapchainExtent2D.height,
			1);

		pCoreBase->camera->viewUpdated = false;
	}

	//copy ray tracing output to swap chain image
	//prepare current swap chain image as transfer destination
	vrt::Tools::setImageLayout(
		pCoreBase->commandBuffers.graphics[frame],
		pCoreBase->swapchainData.swapchainImages.image[frame],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

	//prepare ray tracing output image as transfer source
	vrt::Tools::setImageLayout(
		pCoreBase->commandBuffers.graphics[frame],
		storageImage.image,
		VK_IMAGE_LAYOUT_GENERAL,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		subresourceRange);

	VkImageCopy copyRegion{};
	copyRegion.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.srcOffset = { 0, 0, 0 };
	copyRegion.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 };
	copyRegion.dstOffset = { 0, 0, 0 };
	copyRegion.extent = { pCoreBase->swapchainData.swapchainExtent2D.width, pCoreBase->swapchainData.swapchainExtent2D.height, 1 };

	vkCmdCopyImage(pCoreBase->commandBuffers.graphics[frame], storageImage.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, pCoreBase->swapchainData.swapchainImages.image[frame],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

	//transition swap chain image back for presentation
	vrt::Tools::setImageLayout(
		pCoreBase->commandBuffers.graphics[frame],
		pCoreBase->swapchainData.swapchainImages.image[frame],
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		subresourceRange);

	//transition ray tracing output image back to general layout
	vrt::Tools::setImageLayout(
		pCoreBase->commandBuffers.graphics[frame],
		storageImage.image,
		VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		VK_IMAGE_LAYOUT_GENERAL,
		subresourceRange);

}

void Raytracing::handleResize() {

	// Delete allocated resources
	vkDestroyImageView(pCoreBase->devices.logical, storageImage.view, nullptr);
	vkDestroyImage(pCoreBase->devices.logical, storageImage.image, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, storageImage.memory, nullptr);

	// Recreate image
	createStorageImage();

	// Update descriptor
	VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL };
	VkWriteDescriptorSet resultImageWrite{};
	resultImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	resultImageWrite.dstSet = pipelineData.descriptorSet;
	resultImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	resultImageWrite.dstBinding = 1;
	resultImageWrite.pImageInfo = &storageImageDescriptor;
	resultImageWrite.descriptorCount = 1;

	vkUpdateDescriptorSets(pCoreBase->devices.logical, 1, &resultImageWrite, 0, VK_NULL_HANDLE);

}

uint64_t Raytracing::getBufferDeviceAddress(VkBuffer buffer) {
	VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
	bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	bufferDeviceAI.buffer = buffer;
	return pCoreBase->coreExtensions->vkGetBufferDeviceAddressKHR(pCoreBase->devices.logical, &bufferDeviceAI);
}

// -- destroy raytracing
void Raytracing::DestroyRaytracing() {

	//texture
	texture.DestroyTexture();

	//descriptor pool
	vkDestroyDescriptorPool(pCoreBase->devices.logical, this->descriptorPool, nullptr);

	//shader binding tables
	buffers.raygenShaderBindingTable.destroy(pCoreBase->devices.logical);
	buffers.hitShaderBindingTable.destroy(pCoreBase->devices.logical);
	buffers.missShaderBindingTable.destroy(pCoreBase->devices.logical);

	//pipeline layout and pipeline
	vkDestroyPipelineLayout(pCoreBase->devices.logical, this->pipelineData.pipelineLayout, nullptr);
	vkDestroyPipeline(pCoreBase->devices.logical, this->pipelineData.pipeline, nullptr);

	//descriptors
	vkDestroyDescriptorSetLayout(pCoreBase->devices.logical, this->pipelineData.descriptorSetLayout, nullptr);

	//shaders
	shader.DestroyShader();

	//storage image
	vkDestroyImageView(pCoreBase->devices.logical, this->storageImage.view, nullptr);
	vkDestroyImage(pCoreBase->devices.logical, this->storageImage.image, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->storageImage.memory, nullptr);

	//buffers
	buffers.ubo.destroy(pCoreBase->devices.logical);

	buffers.instancesBuffer.destroy(pCoreBase->devices.logical);
	buffers.vertexBuffer.destroy(pCoreBase->devices.logical);
	buffers.indexBuffer.destroy(pCoreBase->devices.logical);
	buffers.transformBuffer.destroy(pCoreBase->devices.logical);

	//acceleration structure buffers
	//topLevelAS
	vkDestroyBuffer(pCoreBase->devices.logical, this->topLevelAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->topLevelAS.memory, nullptr);

	//bottomLevelAS
	vkDestroyBuffer(pCoreBase->devices.logical, this->bottomLevelAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->bottomLevelAS.memory, nullptr);

	//acceleration structures
	//top
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->topLevelAS.accelerationStructureKHR, nullptr);

	//bottom
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->bottomLevelAS.accelerationStructureKHR, nullptr);
}
