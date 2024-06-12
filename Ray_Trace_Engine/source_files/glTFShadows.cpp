#include "glTFShadows.hpp"

void glTFShadows::CreateStorageImage(){

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

glTFShadows::glTFShadows() { }

glTFShadows::glTFShadows(CoreBase* coreBase) {
	InitglTFShadows(coreBase);
}

void glTFShadows::InitglTFShadows(CoreBase* coreBase) {

	//init core pointer
	this->pCoreBase = coreBase;

	//init shader
	shader = Shader(coreBase);

	// -- create bottom level acceleration structure
	CreateBLAS();

	// -- create top level acceleration structure
	CreateTLAS();

	// -- create storage image
	//vrt::RTObjects::createStorageImage(this->pCoreBase, &this->storageImage, "glTFShadowstorageImage");
	CreateStorageImage();

	// -- create uniform buffer
	CreateUniformBuffer();

	// -- create shadow raytracing pipeline
	CreateRayTracingPipeline();

	// -- create shader binding tables
	CreateShaderBindingTables();

	// -- create descriptor sets
	CreateDescriptorSets();

	// -- setup strided buffer device address
	SetupBufferRegionAddresses();

	// -- build command buffers
	BuildCommandBuffers();


}

void glTFShadows::CreateBLAS() {

	//instead of a simple triangle, we'll be loading a more complex scene for this example

	//the shaders are accessing the vertex and index buffers of the scene, so the proper
	//usage flag has to be set on the vertex and index buffers for the scene
	VkMemoryPropertyFlags memoryPropertyFlags =
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	//this->tfModel = TFModel(this->pCoreBase, "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/FlightHelmet/glTF/FlightHelmet.gltf");
	//this->tfModel = TFModel(this->pCoreBase, "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/CesiumMan/glTF/CesiumMan.gltf");

	//load model
	this->tfModel = TFModel(this->pCoreBase, "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/vulkanscene_shadow.gltf");
	//this->tfModel = TFModel(this->pCoreBase, "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/reflection_scene.gltf");
	//buffer device addresses
	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

	//get addresses
	vertexBufferDeviceAddress.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, this->tfModel.vertices.verticesBuffer.bufferData.buffer);
	indexBufferDeviceAddress.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, this->tfModel.indices.indicesBuffer.bufferData.buffer);

	//number of triangles
	uint32_t numTriangles = static_cast<uint32_t>(this->tfModel.indices.count) / 3;
	//std::cout << "tf model indices count: " << this->tfModel.indices.count << std::endl;

	//number of vertices
	uint32_t maxVertex = this->tfModel.vertices.count - 1;
	//std::cout << "tf model vertices count: " << this->tfModel.vertices.count << std::endl;

	//acceleration structure geometry
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = maxVertex;
	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(TFModel::Vertex);
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

	//acceleration structure build geometry info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	//acceleration structure build sizes info
	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	//get build sizes
	pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	//create BLAS buffer and memory
	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->bottomLevelAS.memory,
		&this->bottomLevelAS.buffer, &accelerationStructureBuildSizesInfo, "glTFShadowsBLASBuffer");

	//acceleration structure create info
	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = bottomLevelAS.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo,
			nullptr, &bottomLevelAS.accelerationStructureKHR);}, "glTFShadowsBLASAccelerationStructure");

	//create scratch buffer for acceleration structure build
	vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase, &scratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize, "glTFShadowscratchBufferBLAS");

	//acceleration structure built geometry info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = bottomLevelAS.accelerationStructureKHR;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.bufferData.bufferDeviceAddress.deviceAddress;

	//acceleration structure build range info
	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	//build the acceleration structure on the device via a one-time command buffer submission
	//some implementations may support acceleration structure building on the host
	//(VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but prefer device builds
	//create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());

	//end and submit and destroy command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);


	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = bottomLevelAS.accelerationStructureKHR;
	bottomLevelAS.deviceAddress =
		pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
			&accelerationDeviceAddressInfo);

	//destroy scratch buffer
	scratchBuffer.destroy(this->pCoreBase->devices.logical);

}

void glTFShadows::CreateTLAS() {
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

	//buffer for instance data
	vrt::Buffer instancesBuffer;

	//name
	instancesBuffer.bufferData.bufferName = "instancesBuffer";
	instancesBuffer.bufferData.bufferMemoryName = "instancesBufferMemory";

	//create instances buffer and allocate memory
	if (pCoreBase->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&instancesBuffer,
		sizeof(VkAccelerationStructureInstanceKHR), &instance) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create top level acceleration structure instances buffer");
	}

	//get instances buffer device address
	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	instanceDataDeviceAddress.deviceAddress = GetBufferDeviceAddress(instancesBuffer.bufferData.buffer);

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
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

	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->topLevelAS.memory,
		&this->topLevelAS.buffer, &accelerationStructureBuildSizesInfo, "ShadowTopLevelASBuffer");

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

	//create scratch buffer
	vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase,
		&scratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize, "glTFShadowscratchBufferTLAS");

	//acceleration structure build geometry info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = topLevelAS.accelerationStructureKHR;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.bufferData.bufferDeviceAddress.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = 1;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	//build the acceleration structure on the device via a one-time command buffer submission
	//some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
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

	scratchBuffer.destroy(this->pCoreBase->devices.logical);
	instancesBuffer.destroy(this->pCoreBase->devices.logical);
}

void glTFShadows::CreateUniformBuffer() {

	ubo.bufferData.bufferName = "shadowUBOBuffer";
	ubo.bufferData.bufferMemoryName = "shadowUBOBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&ubo, sizeof(UniformData), &uniformData) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create shadow uniform buffer!");
	}

	//if (ubo.map(pCoreBase->devices.logical, sizeof(UniformData), 0) != VK_SUCCESS) {
	//	throw std::invalid_argument("failed to map shadow uniform buffer");
	//}

	//ubo.unmap(pCoreBase->devices.logical);

	//UpdateUniformBuffer(0.0f);

}

void glTFShadows::UpdateUniformBuffer(float deltaTime) {
	//timing for light
	float rotationTime = deltaTime * 0.10f;

	//projection matrix
	uniformData.projInverse = glm::inverse(glm::perspective(glm::radians(pCoreBase->camera->Zoom),
		float(pCoreBase->swapchainData.swapchainExtent2D.width) / float(pCoreBase->swapchainData.swapchainExtent2D.height), 0.1f, 1000.0f));
	//view matrix
	uniformData.viewInverse = glm::inverse(pCoreBase->camera->GetViewMatrix());
	//light position
	uniformData.lightPos = //glm::vec4(0.0f, -5.0f + deltaTime, 1.0f, 1.0f);
		glm::vec4(cos(glm::radians(rotationTime * 360.0f)) * 40.0f, -50.0f + sin(glm::radians(rotationTime * 360.0f))
			* 20.0f, 25.0f + sin(glm::radians(rotationTime * 360.0f)) * 5.0f, 1.0f);
	////std::cout << "Light Position: (" << uniformData.lightPos.x << ", " << uniformData.lightPos.y << ", " << uniformData.lightPos.z << ")" << std::endl;
	// Pass the vertex size to the shader for unpacking vertices
	uniformData.vertexSize = sizeof(TFModel::Vertex);
	memcpy(ubo.bufferData.mapped, &uniformData, sizeof(UniformData));
}

void glTFShadows::CreateRayTracingPipeline() {

		//acceleration structure layout binding
		VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding{};
		accelerationStructureLayoutBinding.binding = 0;
		accelerationStructureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
		accelerationStructureLayoutBinding.descriptorCount = 1;
		accelerationStructureLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

		//storage image layout binding
		VkDescriptorSetLayoutBinding storageImageLayoutBinding{};
		storageImageLayoutBinding.binding = 1;
		storageImageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		storageImageLayoutBinding.descriptorCount = 1;
		storageImageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

		// uniform buffer layout binding
		VkDescriptorSetLayoutBinding uniformBufferLayoutBinding{};
		uniformBufferLayoutBinding.binding = 2;
		uniformBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferLayoutBinding.descriptorCount = 1;
		uniformBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR;

		//vertex buffer layout binding
		VkDescriptorSetLayoutBinding vertexBufferLayoutBinding{};
		vertexBufferLayoutBinding.binding = 3;
		vertexBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		vertexBufferLayoutBinding.descriptorCount = 1;
		vertexBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;

		//index buffer layout binding
		VkDescriptorSetLayoutBinding indexBufferLayoutBinding{};
		indexBufferLayoutBinding.binding = 4;
		indexBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		indexBufferLayoutBinding.descriptorCount = 1;
		indexBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
		
		//bindings array
		std::vector<VkDescriptorSetLayoutBinding> bindings({
			accelerationStructureLayoutBinding,
			storageImageLayoutBinding,
			uniformBufferLayoutBinding,
			vertexBufferLayoutBinding,
			indexBufferLayoutBinding
			});

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
		descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		descriptorSetLayoutCreateInfo.pBindings = bindings.data();

		//create descriptor set layout
		pCoreBase->add([this, &descriptorSetLayoutCreateInfo]()
			{return pCoreBase->objCreate.VKCreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo,
				nullptr, &pipelineData.descriptorSetLayout);},
			"shadowDescriptorSetLayout");
	

		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

		//create pipeline layout
		pCoreBase->add([this, &pipelineLayoutCreateInfo]()
			{return pCoreBase->objCreate.VKCreatePipelineLayout(&pipelineLayoutCreateInfo,
				nullptr, &pipelineData.pipelineLayout);}, "shadowRaytracingPipelineLayout");

		//project directory for loading shader modules
		std::filesystem::path projDirectory = std::filesystem::current_path();

	//	Setup ray tracing shader groups
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	
	// Ray generation group
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/rtshad_raygen.rgen.spv",
			VK_SHADER_STAGE_RAYGEN_BIT_KHR, "rtshad_rayGenShaderModule"));

		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}
	
	// Miss group
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/rtshad_miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR,
			"rtshad_missShaderModule"));

		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
		// Second shader for glTFShadows
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/rtshad_shadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR,
			"rtshad_glTFShadowshaderModule"));
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroups.push_back(shaderGroup);
	}

	
	// Closest hit group
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/rtshad_closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			"rtshadow_closestHitShaderModule"));

		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}
	
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
	rayTracingPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCreateInfo.pStages = shaderStages.data();
	rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCreateInfo.pGroups = shaderGroups.data();
	rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 2;
	rayTracingPipelineCreateInfo.layout = this->pipelineData.pipelineLayout;

	//create raytracing pipeline
	pCoreBase->add([this, &rayTracingPipelineCreateInfo]()
		{return pCoreBase->objCreate.VKCreateRaytracingPipeline(&rayTracingPipelineCreateInfo,
			nullptr, &pipelineData.pipeline);}, "shadowRaytracingPipeline");
}

void glTFShadows::CreateShaderBindingTables(){
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
		raygenShaderBindingTable.bufferData.bufferName = "shadowRaygenShaderBindingTable";
		raygenShaderBindingTable.bufferData.bufferMemoryName = "shadowRaygenShaderBindingTableMemory";
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &raygenShaderBindingTable, handleSize, nullptr)
			!= VK_SUCCESS) {
			throw std::runtime_error("failed to create raygenShaderBindingTable");
		}

		// miss
		missShaderBindingTable.bufferData.bufferName = "shadowMissShaderBindingTable";
		missShaderBindingTable.bufferData.bufferMemoryName = "shadowMissShaderBindingTableMemory";
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &missShaderBindingTable, handleSize * 2, nullptr)
			!= VK_SUCCESS) {
			throw std::runtime_error("failed to create missShaderBindingTable");
		}

		// hit
		hitShaderBindingTable.bufferData.bufferName = "shadowHitShaderBindingTable";
		hitShaderBindingTable.bufferData.bufferMemoryName = "shadowHitShaderBindingTableMemory";
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &hitShaderBindingTable, handleSize, nullptr)
			!= VK_SUCCESS) {
			throw std::runtime_error("failed to create hitShaderBindingTable");
		}

		// copy buffers
		raygenShaderBindingTable.map(pCoreBase->devices.logical);
		memcpy(raygenShaderBindingTable.bufferData.mapped, shaderHandleStorage.data(), handleSize);
		raygenShaderBindingTable.unmap(pCoreBase->devices.logical);

		missShaderBindingTable.map(pCoreBase->devices.logical);
		memcpy(missShaderBindingTable.bufferData.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
		missShaderBindingTable.unmap(pCoreBase->devices.logical);

		hitShaderBindingTable.map(pCoreBase->devices.logical);
		memcpy(hitShaderBindingTable.bufferData.mapped, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize);
		hitShaderBindingTable.unmap(pCoreBase->devices.logical);
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating shader binding table: " << e.what() << std::endl;
		// Handle error (e.g., cleanup resources, rethrow, etc.)
		// You may want to add additional error handling here based on your application's requirements
		throw;
	}
}

void glTFShadows::CreateDescriptorSets() {

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 }
	};

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = 1;

	//create descriptor pool
	pCoreBase->add([this, &descriptorPoolCreateInfo]()
		{return pCoreBase->objCreate.VKCreateDescriptorPool(&descriptorPoolCreateInfo,
			nullptr, &descriptorPool);}, "shadowRaytracingDescriptorPool");


	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
	descriptorSetAllocateInfo.descriptorSetCount =1;

	//create descriptor set
	pCoreBase->add([this, &descriptorSetAllocateInfo]()
		{return pCoreBase->objCreate.VKAllocateDescriptorSet(&descriptorSetAllocateInfo,
			nullptr, &this->pipelineData.descriptorSet);}, "shadowRaytracingDescriptorSet");


	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &topLevelAS.accelerationStructureKHR;

	VkWriteDescriptorSet accelerationStructureWrite{};
	accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	// The specialized acceleration structure descriptor has to be chained
	accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
	accelerationStructureWrite.dstSet = pipelineData.descriptorSet;
	accelerationStructureWrite.dstBinding = 0;
	accelerationStructureWrite.descriptorCount = 1;
	accelerationStructureWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

	VkDescriptorImageInfo storageImageDescriptor{ VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL };

	//storage/result image write
	VkWriteDescriptorSet storageImageWrite{};
	storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	storageImageWrite.dstSet = pipelineData.descriptorSet;
	storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	storageImageWrite.dstBinding = 1;
	storageImageWrite.pImageInfo = &storageImageDescriptor;
	storageImageWrite.descriptorCount = 1;


	//ubo descriptor info
	VkDescriptorBufferInfo uboDescriptor{};
	uboDescriptor.buffer = ubo.bufferData.buffer;
	uboDescriptor.offset = 0;
	uboDescriptor.range = ubo.bufferData.size;

	//ubo descriptor write info
	VkWriteDescriptorSet uniformBufferWrite{};
	uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	uniformBufferWrite.dstSet = pipelineData.descriptorSet;
	uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uniformBufferWrite.dstBinding = 2;
	uniformBufferWrite.pBufferInfo = &uboDescriptor;
	uniformBufferWrite.descriptorCount = 1;


	//vertex descriptor info
	VkDescriptorBufferInfo vertexBufferDescriptor{ tfModel.vertices.verticesBuffer.bufferData.buffer, 0, tfModel.vertices.verticesBuffer.bufferData.size };

	//vertex descriptor write info
	VkWriteDescriptorSet vertexBufferWrite{};
	vertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vertexBufferWrite.dstSet = pipelineData.descriptorSet;
	vertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertexBufferWrite.dstBinding = 3;
	vertexBufferWrite.pBufferInfo = &vertexBufferDescriptor;
	vertexBufferWrite.descriptorCount = 1;

	//index descriptor info
	VkDescriptorBufferInfo indexBufferDescriptor{ tfModel.indices.indicesBuffer.bufferData.buffer, 0, tfModel.indices.indicesBuffer.bufferData.size };

	//index descriptor write info
	VkWriteDescriptorSet indexBufferWrite{};
	indexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	indexBufferWrite.dstSet = pipelineData.descriptorSet;
	indexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	indexBufferWrite.dstBinding = 4;
	indexBufferWrite.pBufferInfo = &indexBufferDescriptor;
	indexBufferWrite.descriptorCount = 1;

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		accelerationStructureWrite,
		storageImageWrite,
		uniformBufferWrite,
		vertexBufferWrite,
		indexBufferWrite
	};

	vkUpdateDescriptorSets(pCoreBase->devices.logical, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void glTFShadows::SetupBufferRegionAddresses() {
	//setup buffer regions pointing to shaders in shader binding table
	const uint32_t handleSizeAligned = vrt::Tools::alignedSize(
		pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
		pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleAlignment);

	//VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	raygenStridedDeviceAddressRegion.deviceAddress = GetBufferDeviceAddress(raygenShaderBindingTable.bufferData.buffer);
	raygenStridedDeviceAddressRegion.stride = handleSizeAligned;
	raygenStridedDeviceAddressRegion.size = handleSizeAligned;

	//VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	missStridedDeviceAddressRegion.deviceAddress = GetBufferDeviceAddress(missShaderBindingTable.bufferData.buffer);
	missStridedDeviceAddressRegion.stride = handleSizeAligned;
	missStridedDeviceAddressRegion.size = handleSizeAligned * 2;

	//VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitStridedDeviceAddressRegion.deviceAddress = GetBufferDeviceAddress(hitShaderBindingTable.bufferData.buffer);
	hitStridedDeviceAddressRegion.stride = handleSizeAligned;
	hitStridedDeviceAddressRegion.size = handleSizeAligned;
}

void glTFShadows::BuildCommandBuffers() {
	VkCommandBufferBeginInfo cmdBufInfo{};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	for (int32_t i = 0; i < pCoreBase->commandBuffers.graphics.size(); ++i) {

		//std::cout << " command buffers [" << i << "]" << std::endl;

		if (vkBeginCommandBuffer(pCoreBase->commandBuffers.graphics[i], &cmdBufInfo) != VK_SUCCESS) {
			throw std::invalid_argument("failed to begin recording graphics command buffer");
		}

		////setup buffer regions pointing to shaders in shader binding table
		//const uint32_t handleSizeAligned = vrt::Tools::alignedSize(
		//	pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
		//	pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleAlignment);

		////VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
		//raygenStridedDeviceAddressRegion.deviceAddress = GetBufferDeviceAddress(raygenShaderBindingTable.bufferData.buffer);
		//raygenStridedDeviceAddressRegion.stride = handleSizeAligned;
		//raygenStridedDeviceAddressRegion.size = handleSizeAligned;

		////VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
		//missStridedDeviceAddressRegion.deviceAddress = GetBufferDeviceAddress(missShaderBindingTable.bufferData.buffer);
		//missStridedDeviceAddressRegion.stride = handleSizeAligned;
		//missStridedDeviceAddressRegion.size = handleSizeAligned * 2;

		////VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
		//hitStridedDeviceAddressRegion.deviceAddress = GetBufferDeviceAddress(hitShaderBindingTable.bufferData.buffer);
		//hitStridedDeviceAddressRegion.stride = handleSizeAligned;
		//hitStridedDeviceAddressRegion.size = handleSizeAligned;

		//dispatch the ray tracing commands
		vkCmdBindPipeline(pCoreBase->commandBuffers.graphics[i], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipeline);

		vkCmdBindDescriptorSets(pCoreBase->commandBuffers.graphics[i],
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipelineLayout, 0, 1, &pipelineData.descriptorSet, 0, nullptr);

		VkStridedDeviceAddressRegionKHR emptyShaderSbtEntry{};
		pCoreBase->coreExtensions->vkCmdTraceRaysKHR(
			pCoreBase->commandBuffers.graphics[i],
			&raygenStridedDeviceAddressRegion,
			&missStridedDeviceAddressRegion,
			&hitStridedDeviceAddressRegion,
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
void glTFShadows::RebuildCommandBuffers(uint32_t frame) {
	//handle resize if window is resized
	//if (resized) {
	//	handleResize();
	//}


		//subresource range
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

	//if (pCoreBase->camera->viewUpdated) {

		VkStridedDeviceAddressRegionKHR emptySbtEntry{};

		//dispatch the ray tracing commands
		vkCmdBindPipeline(pCoreBase->commandBuffers.graphics[frame], VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipeline);

		vkCmdBindDescriptorSets(pCoreBase->commandBuffers.graphics[frame],
			VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, pipelineData.pipelineLayout, 0, 1, &pipelineData.descriptorSet, 0, 0);

		pCoreBase->coreExtensions->vkCmdTraceRaysKHR(
			pCoreBase->commandBuffers.graphics[frame],
			&raygenStridedDeviceAddressRegion,
			&missStridedDeviceAddressRegion,
			&hitStridedDeviceAddressRegion,
			&emptySbtEntry,
			pCoreBase->swapchainData.swapchainExtent2D.width,
			pCoreBase->swapchainData.swapchainExtent2D.height,
			1);

	//	pCoreBase->camera->viewUpdated = false;
	//}

	//copy ray tracing output to swap chain image
	//prepare current swap chain image as transfer destination
	vrt::Tools::setImageLayout(
		pCoreBase->commandBuffers.graphics[frame],
		pCoreBase->swapchainData.swapchainImages.image[frame],
		VK_IMAGE_LAYOUT_UNDEFINED,
		VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		subresourceRange);

		////std::cout << "test"<< std::endl;
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
void glTFShadows::DestroyglTFShadows() {

	//descriptor pool
	vkDestroyDescriptorPool(pCoreBase->devices.logical, this->descriptorPool, nullptr);

	//shader binding tables
	raygenShaderBindingTable.destroy(pCoreBase->devices.logical);
	hitShaderBindingTable.destroy(pCoreBase->devices.logical);
	missShaderBindingTable.destroy(pCoreBase->devices.logical);

	//pipeline layout and pipeline
	vkDestroyPipelineLayout(pCoreBase->devices.logical, this->pipelineData.pipelineLayout, nullptr);
	vkDestroyPipeline(pCoreBase->devices.logical, this->pipelineData.pipeline, nullptr);

	//shaders
	shader.DestroyShader();

	//descriptor set layout
	vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, this->pipelineData.descriptorSetLayout, nullptr);

	//uniform buffer
	ubo.destroy(this->pCoreBase->devices.logical);

	//storage image
	vkDestroyImageView(pCoreBase->devices.logical, this->storageImage.view, nullptr);
	vkDestroyImage(pCoreBase->devices.logical, this->storageImage.image, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->storageImage.memory, nullptr);

	//TLAS 
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->topLevelAS.accelerationStructureKHR, nullptr);

	//tlas buffers
	vkDestroyBuffer(pCoreBase->devices.logical, this->topLevelAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->topLevelAS.memory, nullptr);

	//BLAS
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->bottomLevelAS.accelerationStructureKHR, nullptr);

	//blas buffers
	vkDestroyBuffer(pCoreBase->devices.logical, this->bottomLevelAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->bottomLevelAS.memory, nullptr);

	//model
	tfModel.DestroyModel();

}
