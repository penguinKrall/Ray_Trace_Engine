#include "Multi_Model.hpp"

Multi_Model::Multi_Model()
{
}

Multi_Model::Multi_Model(CoreBase* coreBase) {

	Init_Multi_Model(coreBase);

}

void Multi_Model::Init_Multi_Model(CoreBase* coreBase) {

	//init core pointer
	this->pCoreBase = coreBase;

	//shader
	shader = Shader(pCoreBase);

	//load assets
	this->LoadAssets();

	this->PreTransformModel();

	vkDeviceWaitIdle(this->pCoreBase->devices.logical);

	//compute
	this->gltf_compute = glTF_Compute(pCoreBase, this->assets.animatedModel);

	//create bottom level acceleration structure
	this->CreateBLAS();

	//create top level acceleration structure
	this->CreateTLAS();

	//create storage image
	this->CreateStorageImage();

	//create uniform buffer
	this->CreateUniformBuffer();

	//create ray tracing pipeline
	this->CreateRayTracingPipeline();

	//create shader binding tables
	this->CreateShaderBindingTable();

	//create descriptor set
	this->CreateDescriptorSet();

	//setup buffer region device addresses
	this->SetupBufferRegionAddresses();

	//build command buffers
	this->BuildCommandBuffers();


}

void Multi_Model::LoadAssets() {

	const uint32_t glTFLoadingFlags =
		vkglTF::FileLoadingFlags::PreTransformVertices |
		vkglTF::FileLoadingFlags::PreMultiplyVertexColors;

	this->assets.animatedModel = new vkglTF::Model(pCoreBase);
	this->assets.animatedModel->loadFromFile("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/CesiumMan/glTF/CesiumMan.gltf",
		pCoreBase, pCoreBase->queue.graphics);

	this->assets.sceneModel = new vkglTF::Model(pCoreBase);
	this->assets.sceneModel->loadFromFile("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/reflection_scene.gltf",
		pCoreBase, pCoreBase->queue.graphics, glTFLoadingFlags, 1.0f);

}

void Multi_Model::CreateBLAS() {

	std::vector<GeometryNode> geometryNodes{};

	for (auto& node : this->assets.animatedModel->linearNodes) {
		if (node->mesh) {

			std::cout << "\n\n\nreflection blas\n\n\n\n" << std::endl;

			for (auto& primitive : node->mesh->primitives) {
				if (primitive->indexCount > 0) {

					VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress;
					VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress;

					vertexBufferDeviceAddress.deviceAddress =
						vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase,
							this->assets.animatedModel->vertices.buffer.bufferData.buffer);// +primitive.firstVertex * sizeof(vkglTF::Vertex);
					indexBufferDeviceAddress.deviceAddress =
						vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase,
							this->assets.animatedModel->indices.buffer.bufferData.buffer);// +primitive->firstIndex * sizeof(uint32_t);

					VkAccelerationStructureGeometryKHR geometry{};
					geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
					geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
					geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
					geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
					geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
					geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(this->assets.animatedModel->modelVertexBuffer.size());
					std::cout << "this->assets.animatedModel->vertices.count: " << this->assets.animatedModel->vertices.count << std::endl;
					geometry.geometry.triangles.vertexStride = sizeof(vkglTF::Vertex);
					geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
					geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
					blasData.geometries.push_back(geometry);
					blasData.maxPrimitiveCounts.push_back(primitive->indexCount / 3);
					blasData.maxPrimCount += primitive->indexCount / 3;

					VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
					buildRangeInfo.firstVertex = 0;
					buildRangeInfo.primitiveOffset = 0; // primitive.firstIndex * sizeof(uint32_t);
					buildRangeInfo.primitiveCount = primitive->indexCount / 3;
					std::cout << "animated model buildRangeInfo.primitiveCount" << buildRangeInfo.primitiveCount << std::endl;
					buildRangeInfo.transformOffset = 0;
					blasData.buildRangeInfos.push_back(buildRangeInfo);

					GeometryNode geometryNode{};
					geometryNode.vertexBufferDeviceAddress = vertexBufferDeviceAddress.deviceAddress;
					geometryNode.indexBufferDeviceAddress = indexBufferDeviceAddress.deviceAddress;
					geometryNode.textureIndexBaseColor = primitive->material.baseColorTexture ? primitive->material.baseColorTexture->index : -1;
					geometryNode.textureIndexOcclusion = primitive->material.occlusionTexture ? primitive->material.occlusionTexture->index : -1;
					geometryNodes.push_back(geometryNode);
				}
			}
		}
	}

	VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress;
	VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress;

	vertexBufferDeviceAddress.deviceAddress =
		vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase,
			this->assets.sceneModel->vertices.buffer.bufferData.buffer);

	indexBufferDeviceAddress.deviceAddress =
		vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase,
			this->assets.sceneModel->indices.buffer.bufferData.buffer);

	VkAccelerationStructureGeometryKHR geometry{};
	geometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	geometry.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR;
	geometry.geometry.triangles.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
	geometry.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
	geometry.geometry.triangles.vertexData = vertexBufferDeviceAddress;
	geometry.geometry.triangles.maxVertex = static_cast<uint32_t>(this->assets.sceneModel->modelVertexBuffer.size() - 1);
	std::cout << "this->assets.sceneModel->vertices.count: " << this->assets.sceneModel->vertices.count << std::endl;
	geometry.geometry.triangles.vertexStride = sizeof(vkglTF::Vertex);
	geometry.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
	geometry.geometry.triangles.indexData = indexBufferDeviceAddress;
	blasData.geometries.push_back(geometry);
	blasData.maxPrimitiveCounts.push_back(static_cast<uint32_t>(this->assets.sceneModel->indices.count) / 3);
	blasData.maxPrimCount += static_cast<uint32_t>(this->assets.sceneModel->indices.count) / 3;

	VkAccelerationStructureBuildRangeInfoKHR buildRangeInfo{};
	buildRangeInfo.firstVertex = 0;
	buildRangeInfo.primitiveOffset = 0;
	buildRangeInfo.primitiveCount = static_cast<uint32_t>(this->assets.sceneModel->indices.count) / 3;
	std::cout << "scene model buildRangeInfo.primitiveCount" << buildRangeInfo.primitiveCount << std::endl;
	buildRangeInfo.transformOffset = 0;
	blasData.buildRangeInfos.push_back(buildRangeInfo);

	GeometryNode geometryNode{};
	geometryNode.vertexBufferDeviceAddress = vertexBufferDeviceAddress.deviceAddress;
	geometryNode.indexBufferDeviceAddress = indexBufferDeviceAddress.deviceAddress;
	geometryNode.textureIndexBaseColor = -1;
	geometryNode.textureIndexOcclusion = -1;
	geometryNodes.push_back(geometryNode);

	for (auto& rangeInfo : blasData.buildRangeInfos) {
		blasData.pBuildRangeInfos.push_back(&rangeInfo);
	}

	buffers.geometryNodesBuffer.bufferData.bufferName = "glTFAnimation_GeometryNodesBuffer";
	buffers.geometryNodesBuffer.bufferData.bufferMemoryName = "glTFAnimation_GeometryNodesBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.geometryNodesBuffer,
		static_cast<uint32_t>(geometryNodes.size()) * sizeof(GeometryNode),
		geometryNodes.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create glTFAnimation geometry nodes buffer");
	}

	// Get size info
	//acceleration structure build geometry info
	blasData.accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	blasData.accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	blasData.accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	blasData.accelerationStructureBuildGeometryInfo.geometryCount = static_cast<uint32_t>(blasData.geometries.size());
	blasData.accelerationStructureBuildGeometryInfo.pGeometries = blasData.geometries.data();

	blasData.accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
	this->pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		this->pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&blasData.accelerationStructureBuildGeometryInfo,
		blasData.maxPrimitiveCounts.data(),
		&blasData.accelerationStructureBuildSizesInfo);

	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->BLAS.memory, &this->BLAS.buffer,
		&blasData.accelerationStructureBuildSizesInfo, "glTFAnimation_AccelerationStructureBuffer");

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = BLAS.buffer;
	accelerationStructureCreateInfo.size = blasData.accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&BLAS.accelerationStructureKHR);}, "glTFAnimation_BLASAccelerationStructureKHR");

	//create scratch buffer for acceleration structure build
	//vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase, &buffers.blas_scratch, blasData.accelerationStructureBuildSizesInfo.buildScratchSize, "glTFAnimation_ScratchBufferBLAS");

	blasData.accelerationStructureBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	blasData.accelerationStructureBuildGeometryInfo.dstAccelerationStructure = this->BLAS.accelerationStructureKHR;
	blasData.accelerationStructureBuildGeometryInfo.scratchData.deviceAddress = buffers.blas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

	const VkAccelerationStructureBuildRangeInfoKHR* buildOffsetInfo = blasData.buildRangeInfos.data();

	//Build the acceleration structure on the device via a one-time command buffer submission
	//create command buffer
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//build BLAS
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&blasData.accelerationStructureBuildGeometryInfo,
		blasData.pBuildRangeInfos.data());

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

void Multi_Model::CreateTLAS() {

	// We flip the matrix [1][1] = -1.0f to accomodate for the glTF up vector
	VkTransformMatrixKHR transformMatrix = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f };


	//// Create a rotation matrix to rotate -90 degrees around the X-axis
	//glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	//
	//// Extract the 3x4 part of the rotation matrix
	//glm::mat3x4 rotationMatrix3x4 = glm::mat3x4(rotationMatrix);
	//
	//// Create a VkTransformMatrixKHR and fill it with the values from the rotation matrix
	//VkTransformMatrixKHR transformMatrix;
	//memcpy(&transformMatrix, glm::value_ptr(rotationMatrix3x4), sizeof(VkTransformMatrixKHR));

	VkAccelerationStructureInstanceKHR instance{};
	instance.transform = transformMatrix;
	instance.instanceCustomIndex = 0;
	instance.mask = 0xFF;
	instance.instanceShaderBindingTableRecordOffset = 0;
	////std::cout << "this->BLAS.deviceAddress" << this->BLAS.deviceAddress << std::endl;
	instance.accelerationStructureReference = this->BLAS.deviceAddress;

	//buffer for instance data
	/*vrt::Buffer instancesBuffer;*/

	buffers.tlas_instancesBuffer.bufferData.bufferName = "gltfAnimation_TLASInstancesBuffer";
	buffers.tlas_instancesBuffer.bufferData.bufferMemoryName = "gltfAnimation_TLASInstancesBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.tlas_instancesBuffer,
		sizeof(VkAccelerationStructureInstanceKHR),
		&instance) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create glTFAnimation instances buffer");
	}

	//instance buffer device address
	//VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	tlasData.instanceDataDeviceAddress.deviceAddress = vrt::RTObjects::getBufferDeviceAddress(this->pCoreBase, buffers.tlas_instancesBuffer.bufferData.buffer);

	//VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	tlasData.accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	tlasData.accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	tlasData.accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	tlasData.accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	tlasData.accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	tlasData.accelerationStructureGeometry.geometry.instances.data = tlasData.instanceDataDeviceAddress;

	// Get size info
	/*
	The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of pBuildInfo are ignored.
	Any VkDeviceOrHostAddressKHR members of pBuildInfo are ignored by this command, except that the hostAddress
	member of VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be examined to check if it is NULL.*
	*/
	//VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
	tlasData.accelerationStructureBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	tlasData.accelerationStructureBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	tlasData.accelerationStructureBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	tlasData.accelerationStructureBuildGeometryInfo.geometryCount = 1;
	tlasData.accelerationStructureBuildGeometryInfo.pGeometries = &tlasData.accelerationStructureGeometry;

	uint32_t primitive_count = 1;

	//VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	tlasData.accelerationStructureBuildSizesInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

	pCoreBase->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
		pCoreBase->devices.logical,
		VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
		&tlasData.accelerationStructureBuildGeometryInfo,
		&primitive_count,
		&tlasData.accelerationStructureBuildSizesInfo);

	vrt::RTObjects::createAccelerationStructureBuffer(this->pCoreBase, &this->TLAS.memory, &this->TLAS.buffer, &tlasData.accelerationStructureBuildSizesInfo,
		"glTFAnimation_TLASBuffer");

	VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
	accelerationStructureCreateInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
	accelerationStructureCreateInfo.buffer = this->TLAS.buffer;
	accelerationStructureCreateInfo.size = tlasData.accelerationStructureBuildSizesInfo.accelerationStructureSize;
	accelerationStructureCreateInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

	//create acceleration structure
	pCoreBase->add([this, &accelerationStructureCreateInfo]()
		{return pCoreBase->objCreate.VKCreateAccelerationStructureKHR(&accelerationStructureCreateInfo, nullptr,
			&TLAS.accelerationStructureKHR);}, "gltfAnimation_accelerationStructureKHR");

	//create scratch buffer
	//vrt::Buffer scratchBuffer;
	vrt::RTObjects::createScratchBuffer(this->pCoreBase,
		&buffers.tlas_scratch, tlasData.accelerationStructureBuildSizesInfo.buildScratchSize, "glTFAnimation_ScratchBufferTLAS");

	//VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
	tlasData.accelerationBuildGeometryInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
	tlasData.accelerationBuildGeometryInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	tlasData.accelerationBuildGeometryInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
	tlasData.accelerationBuildGeometryInfo.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
	tlasData.accelerationBuildGeometryInfo.dstAccelerationStructure = this->TLAS.accelerationStructureKHR;
	tlasData.accelerationBuildGeometryInfo.geometryCount = 1;
	tlasData.accelerationBuildGeometryInfo.pGeometries = &tlasData.accelerationStructureGeometry;
	tlasData.accelerationBuildGeometryInfo.scratchData.deviceAddress = buffers.tlas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

	VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
	accelerationStructureBuildRangeInfo.primitiveCount = 1;
	accelerationStructureBuildRangeInfo.primitiveOffset = 0;
	accelerationStructureBuildRangeInfo.firstVertex = 0;
	accelerationStructureBuildRangeInfo.transformOffset = 0;

	//std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

	tlasData.accelerationBuildStructureRangeInfos = { &accelerationStructureBuildRangeInfo };

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
		&tlasData.accelerationBuildGeometryInfo,
		tlasData.accelerationBuildStructureRangeInfos.data());

	//flush command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//get acceleration structure device address
	VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
	accelerationDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
	accelerationDeviceAddressInfo.accelerationStructure = this->TLAS.accelerationStructureKHR;

	this->TLAS.deviceAddress = pCoreBase->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(pCoreBase->devices.logical,
		&accelerationDeviceAddressInfo);



	//destroy scratch buffer
	//scratchBuffer.destroy(this->pCoreBase->devices.logical);

	//destroy instances buffer
	//instancesBuffer.destroy(this->pCoreBase->devices.logical);

}

void Multi_Model::CreateStorageImage() {

	vrt::RTObjects::createStorageImage(this->pCoreBase, &this->storageImage, "glTFAnimation_storageImage");

}

void Multi_Model::CreateUniformBuffer() {

	buffers.ubo.bufferData.bufferName = "shadowUBOBuffer";
	buffers.ubo.bufferData.bufferMemoryName = "shadowUBOBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&buffers.ubo, sizeof(UniformData), &uniformData) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create shadow uniform buffer!");
	}

	UpdateUniformBuffer(0.0f);

}

void Multi_Model::UpdateUniformBuffer(float deltaTime) {
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
	uniformData.lightPos =
		glm::vec4(cos(glm::radians(rotationTime * 360.0f)) * 40.0f,
			-20.0f + sin(glm::radians(rotationTime * 360.0f)) * 20.0f,
			25.0f + sin(glm::radians(rotationTime * 360.0f)) * 5.0f,
			0.0f);
	// This value is used to accumulate multiple frames into the finale picture
	// It's required as ray tracing needs to do multiple passes for transparency
	// In this sample we use noise offset by this frame index to shoot rays for transparency into different directions
	// Once enough frames with random ray directions have been accumulated, it looks like proper transparency
	uniformData.frame++;
	memcpy(buffers.ubo.bufferData.mapped, &uniformData, sizeof(UniformData));
}

void Multi_Model::CreateRayTracingPipeline() {

	uint32_t imageCount{ 0 };
	imageCount = static_cast<uint32_t>(this->assets.animatedModel->textures.size());

	//std::cout << "gltfAnimation raytracing pipeline_  imagecount: " << imageCount << std::endl;

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

	//texture image layout binding
	VkDescriptorSetLayoutBinding allTextureImagesLayoutBinding = vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
		5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR, nullptr);


	//bindings array
	std::vector<VkDescriptorSetLayoutBinding> bindings({
		accelerationStructureLayoutBinding,
		storageImageLayoutBinding,
		uniformBufferLayoutBinding,
		textureImageLayoutBinding,
		geometryNodeLayoutBinding,
		allTextureImagesLayoutBinding,

		});

	// Unbound set
	VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
	setLayoutBindingFlags.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
	setLayoutBindingFlags.bindingCount = 6;
	std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
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
		"glTFAnimation_DescriptorSetLayout");

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

	// Ray generation group
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/multi_model_raygen.rgen.spv", VK_SHADER_STAGE_RAYGEN_BIT_KHR,
			"gltfAnimation_raygen"));
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
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/multi_model_miss.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR,
			"gltfAnimation_miss"));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		shaderGroups.push_back(shaderGroup);
		// Second shader for glTFShadows
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/multi_model_shadow.rmiss.spv", VK_SHADER_STAGE_MISS_BIT_KHR,
			"gltfAnimation_shadowmiss"));
		shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroups.push_back(shaderGroup);
	}

	// Closest hit group for doing texture lookups
	{
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/multi_model_closesthit.rchit.spv", VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
			"gltfAnimation_closestHit"));
		VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
		shaderGroup.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
		shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
		shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
		shaderGroup.closestHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
		shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
		// This group also uses an anyhit shader for doing transparency (see anyhit.rahit for details)
		shaderStages.push_back(shader.loadShader(projDirectory.string() + "/shaders/compiled/multi_model_anyhit.rahit.spv", VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
			"gltfAnimation_anyhit"));
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
			nullptr, &pipelineData.pipeline);}, "glTFAnimation_RaytracingPipeline");

}

void Multi_Model::CreateShaderBindingTable() {
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

void Multi_Model::CreateDescriptorSet() {

	//image count
	uint32_t imageCount = static_cast<uint32_t>(this->assets.animatedModel->textures.size());

	//std::cout << "!!!!!!!!!!!CREATEDESCRIPTORSETS!!!!!!!!!!!\nstatic_cast<uint32_t>(this->assets.animatedModel->textures.size(): " 
	//	<< static_cast<uint32_t>(this->assets.animatedModel->textures.size()) << std::endl;
	//descriptor pool sizes
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(this->assets.animatedModel->textures.size()) }
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
			nullptr, &this->pipelineData.descriptorPool);}, "glTFAnimation_DescriptorPool");

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
			nullptr, &this->pipelineData.descriptorSet);}, "glTFAnimation_DescriptorSet");

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


	// geometry -- vertex/index descriptor info
	VkDescriptorBufferInfo geometryBufferDescriptor{
		this->buffers.geometryNodesBuffer.bufferData.buffer,
		0, this->buffers.geometryNodesBuffer.bufferData.size };

	//geometry descriptor write info
	VkWriteDescriptorSet geometryBufferWrite{};
	geometryBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	geometryBufferWrite.dstSet = pipelineData.descriptorSet;
	geometryBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	geometryBufferWrite.dstBinding = 4;
	geometryBufferWrite.pBufferInfo = &geometryBufferDescriptor;
	geometryBufferWrite.descriptorCount = 1;

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		// Binding 0: Top level acceleration structure
		accelerationStructureWrite,
		// Binding 1: Ray tracing result image
		storageImageWrite,
		// Binding 2: Uniform data
		uniformBufferWrite,
		// Binding 4: Geometry node information SSBO
		geometryBufferWrite,
	};

	// Image descriptors for the image array
	std::vector<VkDescriptorImageInfo> textureDescriptors{};
	for (auto texture : this->assets.animatedModel->textures) {
		VkDescriptorImageInfo descriptor{};
		descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptor.sampler = texture.sampler;
		descriptor.imageView = texture.view;
		textureDescriptors.push_back(descriptor);
	}

	VkWriteDescriptorSet writeDescriptorImgArray{};
	writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorImgArray.dstBinding = 5;
	writeDescriptorImgArray.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	writeDescriptorImgArray.descriptorCount = imageCount;
	writeDescriptorImgArray.dstSet = this->pipelineData.descriptorSet;
	writeDescriptorImgArray.pImageInfo = textureDescriptors.data();
	writeDescriptorSets.push_back(writeDescriptorImgArray);

	vkUpdateDescriptorSets(this->pCoreBase->devices.logical,
		static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

}

void Multi_Model::SetupBufferRegionAddresses() {

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
	hitStridedDeviceAddressRegion.size = handleSizeAligned * 2;

}

void Multi_Model::BuildCommandBuffers() {

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

void Multi_Model::RebuildCommandBuffers(int frame) {

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

void Multi_Model::UpdateBLAS() {
	// Build
	//via a one-time command buffer submission
	VkCommandBuffer commandBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	//build BLAS
	pCoreBase->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
		commandBuffer,
		1,
		&blasData.accelerationStructureBuildGeometryInfo,
		blasData.pBuildRangeInfos.data());

	//end and submit and destroy command buffer
	pCoreBase->FlushCommandBuffer(commandBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);
}

void Multi_Model::UpdateTLAS() {

	VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
	accelerationStructureGeometry.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
	accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
	accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
	accelerationStructureGeometry.geometry.instances.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
	accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
	accelerationStructureGeometry.geometry.instances.data = tlasData.instanceDataDeviceAddress;

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

// // // // // in case i need this // // // // //

void Multi_Model::UpdateVertexBuffers() {

	////copy from staging buffers
	////create temporary command buffer
	//VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	//
	//VkBufferCopy copyRegion{};
	//copyRegion.srcOffset = 0;
	//copyRegion.dstOffset = 0;
	//
	//copyRegion.size = this->gltf_compute.storageInputBuffer.bufferData.size;
	//
	////std::cout << "this->gltf_compute.storageInputBuffer.bufferData.size: " << this->gltf_compute.storageInputBuffer.bufferData.size << std::endl;
	////std::cout << "this->model->vertices.buffer.bufferData.size: " << this->model->vertices.buffer.bufferData.size << std::endl;
	////std::cout << "vertexBufferSize: " << vertexBufferSize << std::endl;
	//vkCmdCopyBuffer(cmdBuffer, this->gltf_compute.storageInputBuffer.bufferData.buffer, this->model->vertices.buffer.bufferData.buffer, 1, &copyRegion);
	//
	////submit temporary command buffer and destroy command buffer/memory
	//pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

}

void Multi_Model::PreTransformModel() {

	glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 2.0f));
	for (auto& node : this->assets.animatedModel->linearNodes) {
		if (node->mesh) {
			node->matrix *= glm::transpose(rotationMatrix);
			node->matrix *= translationMatrix;
		}
	}

	//for (auto& node : this->assets.sceneModel->nodes) {
	//	//if (node->mesh) {
	//		node->matrix *= glm::transpose(glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f)));
	//	//}
	//}

}

void Multi_Model::Destroy_Multi_Model() {

	// -- descriptor pool
	vkDestroyDescriptorPool(pCoreBase->devices.logical, this->pipelineData.descriptorPool, nullptr);

	// -- shader binding tables
	raygenShaderBindingTable.destroy(pCoreBase->devices.logical);
	hitShaderBindingTable.destroy(pCoreBase->devices.logical);
	missShaderBindingTable.destroy(pCoreBase->devices.logical);

	// -- shaders
	shader.DestroyShader();

	// -- pipeline and layout
	vkDestroyPipeline(this->pCoreBase->devices.logical, this->pipelineData.pipeline, nullptr);
	vkDestroyPipelineLayout(this->pCoreBase->devices.logical, this->pipelineData.pipelineLayout, nullptr);

	// -- descriptor set layout
	vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, this->pipelineData.descriptorSetLayout, nullptr);

	// -- uniform buffer
	buffers.ubo.destroy(this->pCoreBase->devices.logical);

	// -- storage image
	vkDestroyImageView(pCoreBase->devices.logical, this->storageImage.view, nullptr);
	vkDestroyImage(pCoreBase->devices.logical, this->storageImage.image, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->storageImage.memory, nullptr);

	// -- bottom level acceleration structure & related buffers
	//accel. structure
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->BLAS.accelerationStructureKHR, nullptr);
	//scratch buffer
	buffers.blas_scratch.destroy(this->pCoreBase->devices.logical);
	//accel structure buffer and memory
	vkDestroyBuffer(pCoreBase->devices.logical, this->BLAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->BLAS.memory, nullptr);

	// -- top level acceleration structure & related buffers
	//accel. structure
	pCoreBase->coreExtensions->vkDestroyAccelerationStructureKHR(
		pCoreBase->devices.logical, this->TLAS.accelerationStructureKHR, nullptr);
	//scratch buffer
	buffers.tlas_scratch.destroy(this->pCoreBase->devices.logical);
	//instances buffer
	buffers.tlas_instancesBuffer.destroy(this->pCoreBase->devices.logical);
	//accel. structure buffer and memory
	vkDestroyBuffer(pCoreBase->devices.logical, this->TLAS.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, this->TLAS.memory, nullptr);
	//geometry nodes buffer
	this->buffers.geometryNodesBuffer.destroy(this->pCoreBase->devices.logical);
	//transforms buffer
	this->buffers.transformBuffer.destroy(this->pCoreBase->devices.logical);

	// -- compute class
	this->gltf_compute.Destroy_glTF_Compute();

	// -- models
	this->assets.sceneModel->DestroyModel();
	this->assets.animatedModel->DestroyModel();

}