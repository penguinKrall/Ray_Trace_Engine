#include "glTF_Reflections.hpp"

glTF_Reflections::glTF_Reflections() {

}

glTF_Reflections::glTF_Reflections(CoreBase* coreBase) {
	this->Init_glTF_Reflections(coreBase);
}

#define VK_GLTF_MATERIAL_IDS

void glTF_Reflections::CreateTestUBO() {
	ubo.bufferData.bufferName = "testUBOBuffer";
	ubo.bufferData.bufferMemoryName = "testUBOBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&testUBO, sizeof(TestUniformData), &testUniformData) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create test uniform buffer!");
	}
}

void glTF_Reflections::DestroyglTF_Reflections() {

	//destroy compute
	//this->gltf_compute.Destroy_glTF_Compute();

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
	buffers.tlas_scratch.destroy(this->pCoreBase->devices.logical);
	buffers.tlas_instancesBuffer.destroy(this->pCoreBase->devices.logical);
	vkDestroyBuffer(pCoreBase->devices.logical, this->TLAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->TLAS.memory, nullptr);

	//BLAS
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->BLAS.accelerationStructureKHR, nullptr);

	//blas buffers
	buffers.blas_scratch.destroy(this->pCoreBase->devices.logical);
	vkDestroyBuffer(pCoreBase->devices.logical, this->BLAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->BLAS.memory, nullptr);


	//buffers
	this->buffers.geometryNodesBuffer.destroy(this->pCoreBase->devices.logical);
	this->buffers.transformBuffer.destroy(this->pCoreBase->devices.logical);

	//model
	//this->gltf_model->DestroyModel();

	// -- new model
	//this->gltf_model->Destroy_glTF_model();

	//new new model
	model->DestroyModel();
}

void glTF_Reflections::Init_glTF_Reflections(CoreBase* coreBase) {
	//init core pointer
	this->pCoreBase = coreBase;

	//init shader
	shader = Shader(coreBase);

	//load assets
	LoadAssets();

	//INIT COMPUTE
	//this->gltf_compute = glTF_Compute(coreBase, this->model);

	//create BLAS
	CreateBLAS();

	//create TLAS
	CreateTLAS();

	//create storage image
	CreateStorageImage();

	//create uniform buffer
	CreateUniformBuffer();

	////create raytracing pipeline
	CreatePipeline();

	////create shader binding table
	CreateShaderBindingTable();

	////create descriptor set
	CreateDescriptorSet();
	
	////setup buffer device address regions
	SetupBufferRegionAddresses();

	////build command buffers
	BuildCommandBuffers();

	////std::cout << "" << gltf_model->nodes.size() << std::endl;

}

void glTF_Reflections::LoadAssets() {

	//std::cout << "\nCreating glTF_Reflections BLAS\n-------------------------\n" << std::endl;

	// -- init memory property flags
	this->memoryPropertyFlags =
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	const uint32_t glTFLoadingFlags =
		vkglTF::FileLoadingFlags::PreTransformVertices |
		vkglTF::FileLoadingFlags::PreMultiplyVertexColors;

	//this->gltf_model = glTF_model(this->pCoreBase, "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/FlightHelmet/glTF/FlightHelmet.gltf");
	//this->gltf_model = glTF_model(this->pCoreBase, "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/CesiumMan/glTF/CesiumMan.gltf");

	//sacha model loading file
	model = new vkglTF::Model(this->pCoreBase);

	//model->loadFromFile("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/CesiumMan/glTF/CesiumMan.gltf",
	//	pCoreBase, pCoreBase->queue.graphics);

	//model->loadFromFile("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/gltf_direction_cube/gltf_direction_cube_negy.gltf",
	//	pCoreBase, pCoreBase->queue.graphics);

	model->loadFromFile("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/reflection_scene.gltf",
		pCoreBase, pCoreBase->queue.graphics, glTFLoadingFlags, 1.0f);

	//model->loadFromFile("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/FlightHelmet/glTF/FlightHelmet.gltf",
	//	pCoreBase, pCoreBase->queue.graphics, glTFLoadingFlags, 1.0f);
}

void glTF_Reflections::CreateBLAS() {

	// Instead of a simple triangle, we'll be loading a more complex scene for this example
		// The shaders are accessing the vertex and index buffers of the scene, so the proper usage flag has to be set on the vertex and index buffers for the scene
	vkglTF::memoryPropertyFlags =
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};

	vertexBufferDeviceAddress.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, model->vertices.buffer.bufferData.buffer);
	indexBufferDeviceAddress.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, model->indices.buffer.bufferData.buffer);

	uint32_t numTriangles = static_cast<uint32_t>(model->indices.count) / 3;

	// Build
	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	accelerationStructureGeometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	accelerationStructureGeometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
	accelerationStructureGeometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.maxVertex = model->vertices.count;
	accelerationStructureGeometry.geometry.triangles.vertexStride = sizeof(vkglTF::Vertex);
	accelerationStructureGeometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	accelerationStructureGeometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	accelerationStructureGeometry.geometry.triangles.transformData.deviceAddress = 0;
	accelerationStructureGeometry.geometry.triangles.transformData.hostAddress = nullptr;

	// Get size info
	VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationStructureBuildGeometryInfo.geometryCount = 1;
	accelerationStructureBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;

	VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	this->pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&accelerationStructureBuildGeometryInfo,
		&numTriangles,
		&accelerationStructureBuildSizesInfo);

	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->BLAS.memory, &this->BLAS.buffer,
		&accelerationStructureBuildSizesInfo, "glTF_Reflections_AccelerationStructureBuffer");

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = BLAS.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&BLAS.accelerationStructureKHR);}, "glTF_Reflections_BLASAccelerationStructureKHR");

	//create scratch buffer for acceleration structure build
	//vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase, &buffers.blas_scratch, accelerationStructureBuildSizesInfo.buildScratchSize, "glTF_Reflections_ScratchBufferBLAS");

	accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationStructureBuildGeometryInfo.dstAccelerationStructure = this->BLAS.accelerationStructureKHR;
	accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = buffers.blas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = numTriangles;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;
	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	//Build the acceleration structure on the device via a one-time command buffer submission
	//create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//build BLAS
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&accelerationStructureBuildGeometryInfo,
		accelerationBuildStructureRangeInfos.data());

	//end and submit and destroy command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//get acceleration structure device address
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = this->BLAS.accelerationStructureKHR;
	this->BLAS.deviceAddress =
		pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
			&accelerationDeviceAddressInfo);
}

void glTF_Reflections::CreateTLAS() {

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

	buffers.tlas_instancesBuffer.bufferData.bufferName = "gltfAnimation_TLASInstancesBuffer";
	buffers.tlas_instancesBuffer.bufferData.bufferMemoryName = "gltfAnimation_TLASInstancesBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.tlas_instancesBuffer,
		sizeof(VkAccelerationStructureInstanceKHR),
		&instance) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create glTF_Reflections instances buffer");
	}

	//instance buffer device address
	//VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	tlasData.instanceDataDeviceAddress.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, buffers.tlas_instancesBuffer.bufferData.buffer);

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = tlasData.instanceDataDeviceAddress;

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

	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->TLAS.memory, &this->TLAS.buffer, &accelerationStructureBuildSizesInfo,
		"glTF_Reflections_TLASBuffer");

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = this->TLAS.buffer;
	accelerationStructureCreateInfo.size = accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&TLAS.accelerationStructureKHR);}, "gltfAnimation_accelerationStructureKHR");

	//create scratch buffer
	//vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase,
		&buffers.tlas_scratch, accelerationStructureBuildSizesInfo.buildScratchSize, "glTF_Reflections_ScratchBufferTLAS");

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = this->TLAS.accelerationStructureKHR;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = buffers.tlas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = 1;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;

	std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	// Build the acceleration structure on the device via a one-time command buffer submission
	// Some implementations may support acceleration structure building on the host (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands), but we prefer device builds
	//build acceleration structures
	////create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

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

}

void glTF_Reflections::CreateStorageImage() {
	vrt::RTObjects::createStorageImage(this->pCoreBase, &this->storageImage, "glTF_Reflections_storageImage");
}

void glTF_Reflections::CreateUniformBuffer() {

	ubo.bufferData.bufferName = "gltf_reflection_UBOBuffer";
	ubo.bufferData.bufferMemoryName = "gltf_reflection_UBOBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&ubo, sizeof(UniformData), &uniformData) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltf_reflection_ uniform buffer!");
	}

	UpdateUniformBuffer(0.0f);

}

void glTF_Reflections::UpdateUniformBuffer(float deltaTime) {


	float rotationTime = deltaTime * 0.10f;

	//projection matrix
	glm::mat4 proj = glm::perspective(glm::radians(pCoreBase->camera->Zoom),
		float(pCoreBase->swapchainData.swapchainExtent2D.width) / float(pCoreBase->swapchainData.swapchainExtent2D.height), 0.1f, 1000.0f);
	proj[1][1] *= -1.0f;
	uniformData.projInverse = glm::inverse(proj);

	//view matrix
	uniformData.viewInverse = glm::inverse(pCoreBase->camera->GetViewMatrix());

	//light position
	//uniformData.lightPos = glm::vec4(cos(glm::radians(rotationTime * 360.0f)) * 150.0f, 100.0f + sin(glm::radians(rotationTime * 360.0f))
	//	* 50.0f, 100.0f + sin(glm::radians(rotationTime * 360.0f)) * 150.0f, 1.0f);

	uniformData.lightPos = glm::vec4(0.0f, -20.0f, 0.0f, 0.0f);

	uniformData.lightPos =
		glm::vec4(cos(glm::radians(rotationTime * 360.0f)) * 40.0f,
			-20.0f + sin(glm::radians(rotationTime * 360.0f)) * 20.0f,
			25.0f + sin(glm::radians(rotationTime * 360.0f)) * 5.0f,
			0.0f);

	// Pass the vertex size to the shader for unpacking vertices
	uniformData.vertexSize = sizeof(vkglTF::Vertex);
	memcpy(ubo.bufferData.mapped, &uniformData, sizeof(UniformData));
}

void glTF_Reflections::CreatePipeline() {

	//acceleration structure layout binding
	VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr);

	//storage image layout binding
	VkDescriptorSetLayoutBinding storageImageLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr);

	// uniform buffer layout binding
	VkDescriptorSetLayoutBinding uniformBufferLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR, nullptr);

	VkDescriptorSetLayoutBinding vertexBufferLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr);

	VkDescriptorSetLayoutBinding indexBufferLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, nullptr);

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
		"glTF_Reflections_DescriptorSetLayout");

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

	//create pipeline layout
	pCoreBase->add([this, &pipelineLayoutCreateInfo]()
		{return pCoreBase->objCreate.VKCreatePipelineLayout(&pipelineLayoutCreateInfo,
			nullptr, &pipelineData.pipelineLayout);}, "gltf_reflections_RayTracingPipelineLayout");

	/*
		Setup ray tracing shader groups
	*/
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	VkSpecializationMapEntry specializationMapEntry{};
	specializationMapEntry.constantID = 0;
	specializationMapEntry.offset = 0;
	specializationMapEntry.size = sizeof(uint32_t);

	uint32_t maxRecursion = 4;

	VkSpecializationInfo specializationInfo{};
	specializationInfo.mapEntryCount = 1;
	specializationInfo.pMapEntries = &specializationMapEntry;
	specializationInfo.dataSize = sizeof(maxRecursion);
	specializationInfo.pData = &maxRecursion;

	//project directory for loading shader modules
	std::filesystem::path projDirectory = std::filesystem::current_path();

	// Ray generation group
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/reflections_raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			"gltfReflections_raygen"));
		// Pass recursion depth for reflections to ray generation shader via specialization constant
		shaderStages.back().pSpecializationInfo = &specializationInfo;
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
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/reflections_miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR,
			"gltfReflections_miss"));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	// Closest hit group
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/reflections_closestHit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			"gltfReflections_miss"));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
	}

	//	Create the ray tracing pipeline
	VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
	rayTracingPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
	rayTracingPipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
	rayTracingPipelineCreateInfo.pStages = shaderStages.data();
	rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(shaderGroups.size());
	rayTracingPipelineCreateInfo.pGroups = shaderGroups.data();
	rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 4;
	rayTracingPipelineCreateInfo.layout = pipelineData.pipelineLayout;

	//create raytracing pipeline
	pCoreBase->add([this, &rayTracingPipelineCreateInfo]()
		{return pCoreBase->objCreate.VKCreateRaytracingPipeline(&rayTracingPipelineCreateInfo,
			nullptr, &pipelineData.pipeline);}, "glTF_Reflections_RaytracingPipeline");

}

void glTF_Reflections::CreateShaderBindingTable() {

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
		if (pCoreBase->CreateBuffer(bufferUsageFlags, memoryUsageFlags, &missShaderBindingTable, handleSize, nullptr)
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
		memcpy(missShaderBindingTable.bufferData.mapped, shaderHandleStorage.data() + handleSizeAligned, handleSize);
		missShaderBindingTable.unmap(pCoreBase->devices.logical);

		hitShaderBindingTable.map(pCoreBase->devices.logical);
		memcpy(hitShaderBindingTable.bufferData.mapped, shaderHandleStorage.data() + handleSizeAligned * 2, handleSize);
		hitShaderBindingTable.unmap(pCoreBase->devices.logical);
	}
	catch (const std::exception& e) {
		std::cerr << "Error creating shader binding table: " << e.what() << std::endl;
		// Handle error (e.g., cleanup resources, rethrow, etc.)
		// You may want to add additional error handling here based on your application's requirements
		throw;
	}
}

void glTF_Reflections::CreateDescriptorSet() {

	//image count
	uint32_t imageCount = static_cast<uint32_t>(this->model->textures.size());

	//std::cout << "!!!!!!!!!!!CREATEDESCRIPTORSETS!!!!!!!!!!!\nstatic_cast<uint32_t>(this->model->textures.size(): " 
	//	<< static_cast<uint32_t>(this->model->textures.size()) << std::endl;
	//descriptor pool sizes
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
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
			nullptr, &this->pipelineData.descriptorPool);}, "glTF_Reflections_DescriptorPool");

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
			nullptr, &this->pipelineData.descriptorSet);}, "glTF_Reflections_DescriptorSet");

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
	VkDescriptorBufferInfo vertexBufferDescriptor{
		this->model->vertices.buffer.bufferData.buffer,
		0, this->model->vertices.buffer.bufferData.size };

	//geometry descriptor write info
	VkWriteDescriptorSet vertexBufferWrite{};
	vertexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	vertexBufferWrite.dstSet = pipelineData.descriptorSet;
	vertexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertexBufferWrite.dstBinding = 3;
	vertexBufferWrite.pBufferInfo = &vertexBufferDescriptor;
	vertexBufferWrite.descriptorCount = 1;

	// geometry -- vertex/index descriptor info
	VkDescriptorBufferInfo indexBufferDescriptor{
		this->model->indices.buffer.bufferData.buffer,
		0, this->model->indices.buffer.bufferData.size };

	//geometry descriptor write info
	VkWriteDescriptorSet indexBufferWrite{};
	indexBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	indexBufferWrite.dstSet = pipelineData.descriptorSet;
	indexBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	indexBufferWrite.dstBinding = 4;
	indexBufferWrite.pBufferInfo = &indexBufferDescriptor;
	indexBufferWrite.descriptorCount = 1;


	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		// Binding 0: Top level acceleration structure
		accelerationStructureWrite,
		// Binding 1: Ray tracing result image
		storageImageWrite,
		// Binding 2: Uniform data
		uniformBufferWrite,
		// Binding 3: vertex buffer
		vertexBufferWrite,
		// Binding 4: index buffer
		indexBufferWrite,
	};

	vkUpdateDescriptorSets(this->pCoreBase->devices.logical,
		static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

}

void glTF_Reflections::SetupBufferRegionAddresses() {
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
	missStridedDeviceAddressRegion.size = handleSizeAligned;

	//VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
	hitStridedDeviceAddressRegion.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, hitShaderBindingTable.bufferData.buffer);
	hitStridedDeviceAddressRegion.stride = handleSizeAligned;
	hitStridedDeviceAddressRegion.size = handleSizeAligned;
}

void glTF_Reflections::BuildCommandBuffers() {
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

void glTF_Reflections::UpdateVertexBuffers() {

	//copy from staging buffers
	//create temporary command buffer
	VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;

	copyRegion.size = this->gltf_compute.storageInputBuffer.bufferData.size;

	//std::cout << "this->gltf_compute.storageInputBuffer.bufferData.size: " << this->gltf_compute.storageInputBuffer.bufferData.size << std::endl;
	//std::cout << "this->model->vertices.buffer.bufferData.size: " << this->model->vertices.buffer.bufferData.size << std::endl;
	//std::cout << "vertexBufferSize: " << vertexBufferSize << std::endl;
	vkCmdCopyBuffer(cmdBuffer, this->gltf_compute.storageInputBuffer.bufferData.buffer, this->model->vertices.buffer.bufferData.buffer, 1, &copyRegion);

	//submit temporary command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

}

void glTF_Reflections::UpdateBLAS(float timer) {

	//// Build
	////via a one-time command buffer submission
	//VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	//
	////build BLAS
	//pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
	//	commandBuffer,
	//	1,
	//	&blasData.accelerationStructureBuildGeometryInfo,
	//	blasData.pBuildRangeInfos.data());
	//
	////end and submit and destroy command buffer
	//pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

}

void glTF_Reflections::UpdateTLAS() {

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	//accelerationStructureGeometry.geometry.instances.data = tlasData.instanceDataDeviceAddress;

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

	VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	accelerationBuildGeometryInfo.dstAccelerationStructure = this->TLAS.accelerationStructureKHR;
	accelerationBuildGeometryInfo.geometryCount = 1;
	accelerationBuildGeometryInfo.pGeometries = &accelerationStructureGeometry;
	accelerationBuildGeometryInfo.scratchData.deviceAddress = buffers.tlas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

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

}

void glTF_Reflections::RebuildCommandBuffers(int frame) {

	//subresource range
	VkImageSubresourceRange subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

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
