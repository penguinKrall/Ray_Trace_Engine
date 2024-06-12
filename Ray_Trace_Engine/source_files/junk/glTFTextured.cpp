#include "glTFTextured.hpp"

#define VK_GLTF_MATERIAL_IDS

void glTFTextured::CreateTestUBO() {
	ubo.bufferData.bufferName = "testUBOBuffer";
	ubo.bufferData.bufferMemoryName = "testUBOBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&testUBO, sizeof(TestUniformData), &testUniformData) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create test uniform buffer!");
	}
}

glTFTextured::glTFTextured()
{
}

glTFTextured::glTFTextured(CoreBase* coreBase) {
	InitglTFTextured(coreBase);
}

void glTFTextured::DestroyglTFTextured() {

	
	//descriptor pool
	vkDestroyDescriptorPool(pCoreBase->devices.logical, this->pipelineData.descriptorPool, nullptr);

	//shader binding tables
	raygenShaderBindingTable.destroy(pCoreBase->devices.logical);
	hitShaderBindingTable.destroy(pCoreBase->devices.logical);
	missShaderBindingTable.destroy(pCoreBase->devices.logical);

	//shaders
	shader.DestroyShader();

	//pipeline and layout
	vkDestroyPipeline(this->pCoreBase->devices.logical, this->pipelineData.pipeline, nullptr);
	vkDestroyPipelineLayout(this->pCoreBase->devices.logical, this->pipelineData.pipelineLayout, nullptr);

	//descriptor set layout
	vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, this->pipelineData.descriptorSetLayout, nullptr);

	//test ubo
	testUBO.destroy(this->pCoreBase->devices.logical);
	//uniform buffer
	ubo.destroy(this->pCoreBase->devices.logical);

	//storage image
	vkDestroyImageView(pCoreBase->devices.logical, this->storageImage.view, nullptr);
	vkDestroyImage(pCoreBase->devices.logical, this->storageImage.image, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->storageImage.memory, nullptr);

	//TLAS 
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->TLAS.accelerationStructureKHR, nullptr);

	//tlas buffers
	vkDestroyBuffer(pCoreBase->devices.logical, this->TLAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->TLAS.memory, nullptr);

	//BLAS
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->BLAS.accelerationStructureKHR, nullptr);

	//blas buffers
	vkDestroyBuffer(pCoreBase->devices.logical, this->BLAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->BLAS.memory, nullptr);


	//buffers
	this->buffers.geometryNodesBuffer.destroy(this->pCoreBase->devices.logical);
	this->buffers.transformBuffer.destroy(this->pCoreBase->devices.logical);

	//model
	this->tfModel.DestroyModel();

}

void glTFTextured::InitglTFTextured(CoreBase* coreBase) {
	//init core pointer
	this->pCoreBase = coreBase;

	//init shader
	shader = Shader(coreBase);

	//load assets
	LoadAssets();

	//create BLAS
	CreateBLAS();

	//create TLAS
	CreateTLAS();

	//create storage image
	CreateStorageImage();

	//create uniform buffer
	CreateUniformBuffer();

	//test ubo
	CreateTestUBO();

	//create raytracing pipeline
	CreateRayTracingPipeline();

	//create shader binding table
	CreateShaderBindingTable();

	//create descriptor set
	CreateDescriptorSet();

	//setup buffer device address regions
	SetupBufferRegionAddresses();

	//build command buffers
	BuildCommandBuffers();

	////std::cout << "" << tfModel.nodes.size() << std::endl;

}

void glTFTextured::LoadAssets() {

	//std::cout << "\nCreating glTFTextured BLAS\n-------------------------\n" << std::endl;

	// -- init memory property flags
	this->memoryPropertyFlags =
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	this->tfModel = TFModel(this->pCoreBase, "C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/models/FlightHelmet/glTF/FlightHelmet.gltf");
	//this->tfModel = TFModel(this->pCoreBase, "C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/models/CesiumMan/glTF/CesiumMan.gltf");
}

void glTFTextured::CreateBLAS() {
	//tfModel.UpdateAnimation(0, 0.2f);


	// Use transform matrices from the glTF nodes
	//std::vector<glm::mat4> transformMatrices{};
	/*for (auto node : this->tfModel.linearNodes) {
		//std::cout << "test1" << std::endl;
		if (node.skin != nullptr) {
			//std::cout << "node has mesh" << std::endl;
			//std::cout << "node.name: " << node.name << std::endl;
			//std::cout << "node.mesh.primitives.size(): " << node.mesh.primitives.size() << std::endl;
			for (int i = 0; i < node.mesh.primitives.size(); i++) {
				//std::cout << "test2" << std::endl;
				if (node.mesh.primitives[i].indexCount > 0) {
					//std::cout << "test3" << std::endl;
					//std::cout << "primitive count > 0" << std::endl;
					//std::cout << "node.mesh.primitives[i].indexCount: " << node.mesh.primitives[i].indexCount << std::endl;
					glm::mat4 transformMatrix = node.GetMatrix();
					//std::cout << "node.GetMatrix():" << std::endl;
					for (int row = 0; row < 4; ++row) {
						for (int col = 0; col < 4; ++col) {
							//std::cout << tfModel.modelTransformMatrix[row][col] << " ";
						}
						//std::cout << std::endl;
					}
					transformMatrices.push_back(transformMatrix);
				}
			}
		}
	}*/

	// Use transform matrices from the glTF nodes
	std::vector<glm::mat4> transformMatrices{};
	for (auto node : tfModel.linearNodes) {
		//std::cout << "test1" << std::endl;
		if (node.hasMesh) {
			//std::cout << "test2" << std::endl;
			for (auto primitive : node.mesh.primitives) {
				//std::cout << "test3" << std::endl;
				if (primitive.indexCount > 0) {
					//std::cout << "test4" << std::endl;
					glm::mat4 transformMatrix = glm::mat4(1.0f);
					auto m = glm::mat4(node.GetMatrix());
					memcpy(&transformMatrix, (void*)&m, sizeof(glm::mat4));
					transformMatrices.push_back(transformMatrix);
				}
			}
		}
	}

	//std::cout << "transform matrices size: " << transformMatrices.size() << std::endl;

	// Transform buffer
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.transformBuffer,
		//static_cast<uint32_t>(transformMatrices.size()) * sizeof(VkTransformMatrixKHR),
		static_cast<uint32_t>(transformMatrices.size()) * sizeof(glm::mat4),
		transformMatrices.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create glTFTextured transform buffer");
	}

	// Build
	// One geometry per glTF node, so we can index materials using gl_GeometryIndexEXT
	uint32_t maxPrimCount{ 0 };
	std::vector<uint32_t> maxPrimitiveCounts{};
	std::vector<VkAccelerationStructureGeometryKHR> geometries{};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos{};
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos{};
	std::vector<GeometryNode> geometryNodes{};

	int materialIndex = 0;
	for (auto node : this->tfModel.linearNodes) {
		if (node.hasMesh) {
			for (auto primitive : node.mesh.primitives) {
				//std::cout << "node.mesh.primitives.size(): " << node.mesh.primitives.size() << std::endl;
				if (primitive.indexCount > 0) {
					VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
					VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
					VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};

					vertexBufferDeviceAddress.deviceAddress =
						vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase,
							tfModel.vertices.verticesBuffer.bufferData.buffer);// +primitive.firstVertex * sizeof(vkglTF::Vertex);

					indexBufferDeviceAddress.deviceAddress =
						vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase,
							tfModel.indices.indicesBuffer.bufferData.buffer) + primitive.firstIndex * sizeof(uint32_t);

					transformBufferDeviceAddress.deviceAddress =
						vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase,
							//buffers.transformBuffer.bufferData.buffer) + static_cast<uint32_t>(geometryNodes.size()) * sizeof(VkTransformMatrixKHR);
							buffers.transformBuffer.bufferData.buffer) + static_cast<uint32_t>(geometryNodes.size()) * sizeof(glm::mat4);

					VkAccelerationStructureGeometryKHR geometry{};
					geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
					geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
					geometry.geometry.triangles.maxVertex = tfModel.vertices.count;
					//geometry.geometry.triangles.maxVertex = primitive.vertexCount;
					geometry.geometry.triangles.vertexStride = sizeof(TFModel::Vertex);
					geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
					geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
					geometry.geometry.triangles.transformData = transformBufferDeviceAddress;
					geometries.push_back(geometry);
					maxPrimitiveCounts.push_back(primitive.indexCount / 3);
					maxPrimCount += primitive.indexCount / 3;

					VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
					buildRangeInfo.firstVertex = 0;
					buildRangeInfo.primitiveOffset = 0; // primitive.firstIndex * sizeof(uint32_t);
					buildRangeInfo.primitiveCount = primitive.indexCount / 3;
					buildRangeInfo.transformOffset = 0;
					buildRangeInfos.push_back(buildRangeInfo);

					GeometryNode geometryNode{};
					//std::cout << "node.name" << node.name << std::endl;
					geometryNode.vertexBufferDeviceAddress = vertexBufferDeviceAddress.deviceAddress;
					geometryNode.indexBufferDeviceAddress = indexBufferDeviceAddress.deviceAddress;
					//std::cout << "primitive.material.baseColorTexture->index" << tfModel.materials[materialIndex].baseColorTexture.index << std::endl;
					geometryNode.textureIndexBaseColor = tfModel.materials[materialIndex].baseColorTexture.index;
					geometryNode.textureIndexOcclusion =
						(tfModel.materials[materialIndex].occlusionTexture.index > -1) ? tfModel.materials[materialIndex].occlusionTexture.index : -1;
					geometryNodes.push_back(geometryNode);
					++materialIndex;
				}
			}
		}
	}

	for (auto& rangeInfo : buildRangeInfos) {
		pBuildRangeInfos.push_back(&rangeInfo);
	}

	buffers.geometryNodesBuffer.bufferData.bufferName = "glTFTextured_GeometryNodesBuffer";
	buffers.geometryNodesBuffer.bufferData.bufferMemoryName = "glTFTextured_GeometryNodesBufferMemory";

	// @todo: stage to device
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.geometryNodesBuffer,
		static_cast<uint32_t>(geometryNodes.size()) * sizeof(GeometryNode),
		geometryNodes.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create glTFTextured geometry nodes buffer");
	}

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(geometries.size());
	accelerationStructureBuildGeometryInfo.pGeometries = geometries.data();

	const uint32_t numTriangles = maxPrimitiveCounts[0];

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	this->pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		this->pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		maxPrimitiveCounts.data(),
		&accelerationStructureBuildSizesInfo);

	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->BLAS.memory, &this->BLAS.buffer,
		&accelerationStructureBuildSizesInfo, "glTFTextured_AccelerationStructureBuffer");

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = BLAS.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&BLAS.accelerationStructureKHR);}, "glTFTextured_BLASAccelerationStructureKHR");

	//create scratch buffer for acceleration structure build
	vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase, &scratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize, "glTFTextured_ScratchBufferBLAS");

	accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationStructureBuildGeometryInfo.dstAccelerationStructure = this->BLAS.accelerationStructureKHR;
	accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = scratchBuffer.bufferData.bufferDeviceAddress.deviceAddress;

	const VkAccelerationStructureBuildRangeInfoKHR* buildOffsetInfo = buildRangeInfos.data();

	//Build the acceleration structure on the device via a one-time command buffer submission
	//Some implementations may support acceleration structure building on the host
	//(VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	//create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//build BLAS
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationStructureBuildGeometryInfo,
		pBuildRangeInfos.data());

	//end and submit and destroy command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//get acceleration structure device address
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = this->BLAS.accelerationStructureKHR;
	this->BLAS.deviceAddress =
		pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
			&accelerationDeviceAddressInfo);

	//destroy scratch buffer
	scratchBuffer.destroy(this->pCoreBase->devices.logical);
}

void glTFTextured::CreateTLAS() {
	// We flip the matrix [1][1] = -1.0f to accomodate for the glTF up vector
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
	instance.accelerationStructureReference = this->BLAS.deviceAddress;

	//buffer for instance data
	vrt::Buffer instancesBuffer;

	instancesBuffer.bufferData.bufferName = "glTFTextured_TLASInstancesBuffer";
	instancesBuffer.bufferData.bufferMemoryName = "glTFTextured_TLASInstancesBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&instancesBuffer,
		sizeof(VkAccelerationStructureInstanceKHR),
		&instance) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create glTFTextured instances buffer");
	}

	//instance buffer device address
	VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	instanceDataDeviceAddress.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, instancesBuffer.bufferData.buffer);

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = instanceDataDeviceAddress;

	// Get size info
	/*
	The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored.
	Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress
	member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
	*/
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

	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->TLAS.memory, &this->TLAS.buffer, &accelerationStructureBuildSizesInfo,
		"glTFTextured_TLASBuffer");

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = this->TLAS.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&TLAS.accelerationStructureKHR);}, "glTFTextured_accelerationStructureKHR");

	//create scratch buffer
	vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase,
		&scratchBuffer, accelerationStructureBuildSizesInfo.buildScratchSize, "glTFTextured_ScratchBufferTLAS");

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = this->TLAS.accelerationStructureKHR;
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
	//some implementations may support acceleration structure building on the host
	//(VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	//create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//this is where i start next
	//build acceleration structures
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());

	//flush command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//get acceleration structure device address
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = this->TLAS.accelerationStructureKHR;

	this->TLAS.deviceAddress = pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
		&accelerationDeviceAddressInfo);



	//destroy scratch buffer
	scratchBuffer.destroy(this->pCoreBase->devices.logical);

	//destroy instances buffer
	instancesBuffer.destroy(this->pCoreBase->devices.logical);

}

void glTFTextured::RecreateAccelerationStructures() {

	//vkDeviceWaitIdle(this->pCoreBase->devices.logical);
	//
	////TLAS 
	//pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
	//	pCoreBase->devices.logical, this->TLAS.accelerationStructureKHR, nullptr);
	//
	////tlas buffers
	//vkDestroyBuffer(pCoreBase->devices.logical, this->TLAS.buffer, nullptr);
	//vkFreeMemory(pCoreBase->devices.logical, this->TLAS.memory, nullptr);
	//
	////BLAS
	//pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
	//	pCoreBase->devices.logical, this->BLAS.accelerationStructureKHR, nullptr);
	//
	////blas buffers
	//vkDestroyBuffer(pCoreBase->devices.logical, this->BLAS.buffer, nullptr);
	//vkFreeMemory(pCoreBase->devices.logical, this->BLAS.memory, nullptr);

	CreateBLAS();
	CreateTLAS();
}

void glTFTextured::CreateStorageImage() {
	vrt::RTObjects::createStorageImage(this->pCoreBase, &this->storageImage, "glTFTextured_storageImage");
}

void glTFTextured::CreateUniformBuffer() {

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

	UpdateUniformBuffer(0.0f);
	
}

void glTFTextured::UpdateUniformBuffer(float deltaTime) {


	float rotationTime = deltaTime * 0.10f;

	//projection matrix
	uniformData.projInverse = glm::inverse(glm::perspective(glm::radians(pCoreBase->camera->Zoom),
		float(pCoreBase->swapchainData.swapchainExtent2D.width) / float(pCoreBase->swapchainData.swapchainExtent2D.height), 0.1f, 1000.0f));
	//view matrix
	uniformData.viewInverse = glm::inverse(pCoreBase->camera->GetViewMatrix());
	//light position
	uniformData.lightPos = glm::vec4(cos(glm::radians(rotationTime * 360.0f)) * 40.0f, -50.0f + sin(glm::radians(rotationTime * 360.0f))
			* 20.0f, -25.0f + sin(glm::radians(rotationTime * 360.0f)) * 5.0f, 1.0f);

	//uniformData.testMat = glm::mat4(20.0f);

	// This value is used to accumulate multiple frames into the finale picture
	// It's required as ray tracing needs to do multiple passes for transparency
	// In this sample we use noise offset by this frame index to shoot rays for transparency into different directions
	// Once enough frames with random ray directions have been accumulated, it looks like proper transparency
	//uniformData.frame++;
	memcpy(ubo.bufferData.mapped, &uniformData, sizeof(UniformData));
}

void glTFTextured::CreateRayTracingPipeline() {
	// @todo:
	uint32_t imageCount{ 0 };
	imageCount = static_cast<uint32_t>(tfModel.textures.size());

	//std::cout << "glTFTextured raytracing pipeline_  imagecount: " << imageCount << std::endl;

	//acceleration structure layout binding
	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr);

	//storage image layout binding
	VkDescriptorSetLayoutBinding storageImageLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr);

	// uniform buffer layout binding
	VkDescriptorSetLayoutBinding uniformBufferLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, nullptr);

	//texture image layout binding
	VkDescriptorSetLayoutBinding textureImageLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, nullptr);

	//geometry node layout binding
	VkDescriptorSetLayoutBinding geometryNodeLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, nullptr);

	//skin ssbo layout binding
	VkDescriptorSetLayoutBinding skinSSBOLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr);

	//texture image layout binding
	VkDescriptorSetLayoutBinding allTextureImagesLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, nullptr);


	//bindings array
	std::vector<VkDescriptorSetLayoutBinding> bindings({
		accelerationStructureLayoutBinding,
		storageImageLayoutBinding,
		uniformBufferLayoutBinding,
		textureImageLayoutBinding,
		geometryNodeLayoutBinding,
		skinSSBOLayoutBinding,
		allTextureImagesLayoutBinding,

		});

	// Unbound set
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
	setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	setLayoutBindingFlags.bindingCount = 7;
	std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
		0,
		0,
		0,
		0,
		0,
		0,
		VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
	};

	setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutCreateInfo.pBindings = bindings.data();
	descriptorSetLayoutCreateInfo.pNext = &setLayoutBindingFlags;
	
	//create descriptor set layout
	pCoreBase->add([this, &descriptorSetLayoutCreateInfo]()
		{return pCoreBase->objCreate.VKCreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo,
			nullptr, &pipelineData.descriptorSetLayout);},
		"glTFTextured_DescriptorSetLayout");

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

	//create pipeline layout
	pCoreBase->add([this, &pipelineLayoutCreateInfo]()
		{return pCoreBase->objCreate.VKCreatePipelineLayout(&pipelineLayoutCreateInfo,
			nullptr, &pipelineData.pipelineLayout);}, "gltShadTex_RayTracingPipelineLayout");

	//project directory for loading shader modules
	std::filesystem::path projDirectory = std::filesystem::current_path();

	//Setup ray tracing shader groups
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	
	// Ray generation group
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/tfst_raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			"glTFTextured_raygen"));
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
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/tfst_miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR,
			"glTFTextured_miss"));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
		// Second shader for glTFShadows
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/tfst_shadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR,
			"glTFTextured_shadowmiss"));
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroups.push_back(shaderGroup);
	}
	
	// Closest hit group for doing texture lookups
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/tfst_closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			"glTFTextured_closestHit"));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		// This group also uses an anyhit shader for doing transparency (see anyhit.rahit for details)
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/tfst_anyhit.rahit.spv", VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
			"glTFTextured_anyhit"));
		shaderGroup.anyHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroups.push_back(shaderGroup);
	}
	
	
	//	Create the ray tracing pipeline
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
			nullptr, &pipelineData.pipeline);}, "glTFTextured_RaytracingPipeline");

	//VK_CHECK_RESULT(vkCreateRayTracingPipelinesKHR(device, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rayTracingPipelineCI, nullptr, &pipeline));
}

void glTFTextured::CreateShaderBindingTable(){

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
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &hitShaderBindingTable, handleSize * 2, nullptr)
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
		memcpy(hitShaderBindingTable.bufferData.mapped, shaderHandleStorage.data() + handleSizeAligned * 3, handleSize * 2);
		hitShaderBindingTable.unmap(pCoreBase->devices.logical);
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating shader binding table: " << e.what() << std::endl;
		// Handle error (e.g., cleanup resources, rethrow, etc.)
		// You may want to add additional error handling here based on your application's requirements
		throw;
	}
}

void glTFTextured::CreateDescriptorSet() {

	//image count
	uint32_t imageCount = static_cast<uint32_t>(this->tfModel.textures.size());

	//descriptor pool sizes
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(this->tfModel.textures.size()) }
	};
	//descriptor pool create info
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = 1;

	//create descriptor pool
	pCoreBase->add([this, &descriptorPoolCreateInfo]()
		{return pCoreBase->objCreate.VKCreateDescriptorPool(&descriptorPoolCreateInfo,
			nullptr, &this->pipelineData.descriptorPool);}, "glTFTextured_DescriptorPool");

	VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo{};
	uint32_t variableDescCounts[] = { imageCount };
	variableDescriptorCountAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
	variableDescriptorCountAllocInfo.descriptorSetCount = 1;
	variableDescriptorCountAllocInfo.pDescriptorCounts = variableDescCounts;

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = this->pipelineData.descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
	descriptorSetAllocateInfo.descriptorSetCount = 1;
	descriptorSetAllocateInfo.pNext = &variableDescriptorCountAllocInfo;

	//create descriptor set
	pCoreBase->add([this, &descriptorSetAllocateInfo]()
		{return pCoreBase->objCreate.VKAllocateDescriptorSet(&descriptorSetAllocateInfo,
			nullptr, &this->pipelineData.descriptorSet);}, "glTFTextured_DescriptorSet");

	VkWriteDescriptorSetAccelerationStructureKHR descriptorAccelerationStructureInfo{};
	descriptorAccelerationStructureInfo.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
	descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
	descriptorAccelerationStructureInfo.pAccelerationStructures = &this->TLAS.accelerationStructureKHR;

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


	// geometry -- vertex/index descriptor info
	VkDescriptorBufferInfo geometryBufferDescriptor{ 
		this->buffers.geometryNodesBuffer.bufferData.buffer,
		0, this->buffers.geometryNodesBuffer.bufferData.size};

	//geometry descriptor write info
	VkWriteDescriptorSet geometryBufferWrite{};
	geometryBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	geometryBufferWrite.dstSet = pipelineData.descriptorSet;
	geometryBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	geometryBufferWrite.dstBinding = 4;
	geometryBufferWrite.pBufferInfo = &geometryBufferDescriptor;
	geometryBufferWrite.descriptorCount = 1;

	//skin ssbo descriptor info
	VkDescriptorBufferInfo skinSSBODescriptor{
		this->tfModel.skins[0].ssbo.bufferData.buffer,
		0, this->tfModel.skins[0].ssbo.bufferData.size };

	//skin ssbo descriptor write info
	VkWriteDescriptorSet skinSSBOWrite{};
	skinSSBOWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	skinSSBOWrite.dstSet = pipelineData.descriptorSet;
	skinSSBOWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	skinSSBOWrite.dstBinding = 5;
	skinSSBOWrite.pBufferInfo = &skinSSBODescriptor;
	skinSSBOWrite.descriptorCount = 1;

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		// Binding 0: Top level acceleration structure
		accelerationStructureWrite,
		// Binding 1: Ray tracing result image
		storageImageWrite,
		// Binding 2: Uniform data
		uniformBufferWrite,
		// Binding 4: Geometry node information SSBO
		geometryBufferWrite,
		// Binding 5: Skin SSBO descriptor
		skinSSBOWrite,
	};
	
	// Image descriptors for the image array
	std::vector<VkDescriptorImageInfo> textureDescriptors{};
	for (auto texture : tfModel.textures) {
		VkDescriptorImageInfo descriptor{};
		descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptor.sampler = texture.sampler;
		descriptor.imageView = texture.view;
		textureDescriptors.push_back(descriptor);
	}
	
	VkWriteDescriptorSet writeDescriptorImgArray{};
	writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorImgArray.dstBinding = 6;
	writeDescriptorImgArray.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorImgArray.descriptorCount = imageCount;
	writeDescriptorImgArray.dstSet = this->pipelineData.descriptorSet;
	writeDescriptorImgArray.pImageInfo = textureDescriptors.data();
	writeDescriptorSets.push_back(writeDescriptorImgArray);
	
	vkUpdateDescriptorSets(this->pCoreBase->devices.logical,
		static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void glTFTextured::SetupBufferRegionAddresses() {
	//setup buffer regions pointing to shaders in shader binding table
	const uint32_t handleSizeAligned = vrt::Tools::alignedSize(
		pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
		pCoreBase->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleAlignment);

	//VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
	raygenStridedDeviceAddressRegion.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, raygenShaderBindingTable.bufferData.buffer);
	raygenStridedDeviceAddressRegion.stride = handleSizeAligned;
	raygenStridedDeviceAddressRegion.size = handleSizeAligned;

	//VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
	missStridedDeviceAddressRegion.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, missShaderBindingTable.bufferData.buffer);
	missStridedDeviceAddressRegion.stride = handleSizeAligned;
	missStridedDeviceAddressRegion.size = handleSizeAligned * 2;

	//VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitStridedDeviceAddressRegion.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, hitShaderBindingTable.bufferData.buffer);
	hitStridedDeviceAddressRegion.stride = handleSizeAligned;
	hitStridedDeviceAddressRegion.size = handleSizeAligned;
}

void glTFTextured::BuildCommandBuffers(){
	//if (resized)
	//{
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

void glTFTextured::RebuildCommandBuffers(int frame) {

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