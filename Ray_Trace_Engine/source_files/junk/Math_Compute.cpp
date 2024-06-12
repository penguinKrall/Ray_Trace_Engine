#include "Math_Compute.hpp"

Math_Compute::Math_Compute() {
}

Math_Compute::Math_Compute(CoreBase* coreBase) {

	this->Init_Math_Compute(coreBase);

	this->shader = Shader(coreBase);

	this->CreateBuffers();

	this->CreatePipeline();

	this->CreateDescriptorSets();

	this->CreateCommandBuffers();

	std::vector<ComputeTestData> tempdata{};

	tempdata = this->RetrieveBufferData();

	for (int i = 0; i < tempdata.size(); i++) {
	std::cout << "\ntempdata[" << i << "].testvec4_a.x: " << tempdata[i].testVec4_a.x << std::endl;
	std::cout << "tempdata[" << i << "].testvec4_a.y: " << tempdata[i].testVec4_a.y << std::endl;
	std::cout << "tempdata[" << i << "].testvec4_a.z: " << tempdata[i].testVec4_a.z << std::endl;
	std::cout << "tempdata[" << i << "].testvec4_a.w: " << tempdata[i].testVec4_a.w << std::endl;
	std::cout << "\ntempdata[" << i << "].testvec4_b.x: " << tempdata[i].testVec4_b.x << std::endl;
	std::cout << "tempdata[" << i << "].testvec4_b.y: " << tempdata[i].testVec4_b.y << std::endl;
	std::cout << "tempdata[" << i << "].testvec4_b.z: " << tempdata[i].testVec4_b.z << std::endl;
	std::cout << "tempdata[" << i << "].testvec4_b.w: " << tempdata[i].testVec4_b.w << std::endl;
	}

	//std::cout << "tempdata_ array count: " << tempdata.size() << std::endl;

	//for (int i = 0; i < tempdata.size(); i++) {
	//	std::cout << "tempdata_ retrieved buffer data[" << i << "]: " << tempdata[i] << std::endl;
	//}



}

void Math_Compute::Destroy_Math_Compute() {

	//descriptor pool
	vkDestroyDescriptorPool(this->pCoreBase->devices.logical, this->pipelineData.descriptorPool, nullptr);

	//pipeline and layout
	vkDestroyPipelineLayout(this->pCoreBase->devices.logical, this->pipelineData.pipelineLayout, nullptr);
	vkDestroyPipeline(this->pCoreBase->devices.logical, this->pipelineData.pipeline, nullptr);

	//descriptor set layout
	vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, this->pipelineData.descriptorSetLayout, nullptr);

	//shaders
	this->shader.DestroyShader();

	//buffers
	this->storageInputBuffer.destroy(this->pCoreBase->devices.logical);

}

void Math_Compute::Init_Math_Compute(CoreBase* coreBase) {

	this->pCoreBase = coreBase;

}

void Math_Compute::CreateBuffers() {

	// test Data
	std::cout << "______________ begin test data ______________" << std::endl;
	std::cout << "sizeof(ctData) before vector init: " <<  sizeof(this->ctData) << std::endl;
	//this->ctData.coords = {
	//	0.0f, 1.0f, 2.0f, 3.0f, 4.0f
	//};
	//this->ctData.coords2 = {
	//0.0f, 1.0f, 2.0f, 3.0f, 4.0f
	//};
	//std::cout << "sizeof(ctData) after vector init: " << sizeof(this->ctData) << std::endl;

	std::cout << "______________ end test data ______________" << std::endl;


	//this->coords = {
	//	0.0f, 1.0f, 2.0f, 3.0f, 4.0f
	//};
	this->ctCount = 2;
	this->ctData.resize(ctCount);

	for (int i = 0; i < ctCount; i++) {
	this->ctData[i].testVec4_a = { 1.0f + i, 2.0f + i, 3.0f + i, 4.0f + i };
	this->ctData[i].testVec4_b = { -1.0f - i, -2.0f - i, -3.0f - i, -4.0f - i };

	}

	//std::cout << "coords size: " << this->coords.size() << std::endl;

	//assert((this->coords.size() > 0));

	// -- buffer size
	//VkDeviceSize bufferSize = this->coords.size() * sizeof(float);
	VkDeviceSize bufferSize = sizeof(Math_Compute::ComputeTestData) * ctCount;

	// -- array index/count
	//this->count = this->coords.size();

	//staging buffers
	vrt::Buffer coordsStagingBuffer{};
	coordsStagingBuffer.bufferData.bufferName = "_coordsStagingBufferBuffer";
	coordsStagingBuffer.bufferData.bufferMemoryName = "_coordsStagingBufferBufferMemory";

	// Create staging buffers
	// Vertex data
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&coordsStagingBuffer,
		bufferSize,
		//this->coords.data()) != VK_SUCCESS) {
		ctData.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create Math_Compute coords staging buffer");
	}

	//input buffer
	this->storageInputBuffer.bufferData.bufferName = "math_compute_inputStorageBuffer_";
	this->storageInputBuffer.bufferData.bufferMemoryName = "math_compute_inputStorageBufferMemory_";

	this->pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&this->storageInputBuffer,
		//this->model->modelVertexBuffer.size() * sizeof(vkglTF::Vertex),
		//this->model->modelVertexBuffer.data());
		bufferSize,
		nullptr);

	//copy from staging buffers
	//create temporary command buffer
	VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;

	copyRegion.size = bufferSize;
	std::cout << "vmath_compute input buffer size: " << bufferSize << std::endl;
	//vkCmdCopyBuffer(cmdBuffer, vertexStaging1.bufferData.buffer, this->storageInputBuffer.bufferData.buffer, 1, &copyRegion);
	vkCmdCopyBuffer(cmdBuffer, coordsStagingBuffer.bufferData.buffer, this->storageInputBuffer.bufferData.buffer, 1, &copyRegion);
	//submit temporary command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//destroy staging buffers
	vkDestroyBuffer(pCoreBase->devices.logical, coordsStagingBuffer.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, coordsStagingBuffer.bufferData.memory, nullptr);

}

void Math_Compute::CreatePipeline() {

	//descriptor binding
	VkDescriptorSetLayoutBinding inputStorageBufferLayoutBinding =
		vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT,
			nullptr);

	//array of bindings
	std::vector<VkDescriptorSetLayoutBinding> bindings({
	inputStorageBufferLayoutBinding
		});

	//descriptor set layout create info
	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	descriptorSetLayoutCreateInfo.pBindings = bindings.data();

	//create descriptor set layout
	pCoreBase->add([this, &descriptorSetLayoutCreateInfo]()
		{return pCoreBase->objCreate.VKCreateDescriptorSetLayout(&descriptorSetLayoutCreateInfo,
			nullptr, &pipelineData.descriptorSetLayout);},
		"Math_Compute_DescriptorSetLayout");

	//pipeline layout create info
	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

	//create pipeline layout
	pCoreBase->add([this, &pipelineLayoutCreateInfo]()
		{return pCoreBase->objCreate.VKCreatePipelineLayout(&pipelineLayoutCreateInfo,
			nullptr, &pipelineData.pipelineLayout);}, "Math_Compute_pipelineLayout");

	//project directory for loading shader modules
	std::filesystem::path projDirectory = std::filesystem::current_path();

	//Setup ray tracing shader groups
	VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo;

	computeShaderStageCreateInfo = shader.loadShader(projDirectory.string() + "/shaders/compiled/math_compute.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT,
		"math_compute_shader");
	VkComputePipelineCreateInfo computePipelineCreateInfo = {};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.stage = computeShaderStageCreateInfo;
	computePipelineCreateInfo.layout = this->pipelineData.pipelineLayout;

	if (vkCreateComputePipelines(pCoreBase->devices.logical,
		VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &this->pipelineData.pipeline) != VK_SUCCESS) {
		throw std::invalid_argument("Failed to create math_compute pipeline!");
	}

}

void Math_Compute::CreateDescriptorSets() {

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 }
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
			nullptr, &this->pipelineData.descriptorPool);}, "gltf_compute_DescriptorPool");

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = this->pipelineData.descriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
	descriptorSetAllocateInfo.descriptorSetCount = 1;

	//create descriptor set
	pCoreBase->add([this, &descriptorSetAllocateInfo]()
		{return pCoreBase->objCreate.VKAllocateDescriptorSet(&descriptorSetAllocateInfo,
			nullptr, &this->pipelineData.descriptorSet);}, "gltf_compute_DescriptorSet");

	//storage input buffer descriptor info
	VkDescriptorBufferInfo storageInputBufferDescriptor{};
	storageInputBufferDescriptor.buffer = storageInputBuffer.bufferData.buffer;
	storageInputBufferDescriptor.offset = 0;
	storageInputBufferDescriptor.range = this->storageInputBuffer.bufferData.size;

	//storage input buffer descriptor write info
	VkWriteDescriptorSet storageInputBufferWrite{};
	storageInputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	storageInputBufferWrite.dstSet = pipelineData.descriptorSet;
	storageInputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageInputBufferWrite.dstBinding = 0;
	storageInputBufferWrite.pBufferInfo = &storageInputBufferDescriptor;
	storageInputBufferWrite.descriptorCount = 1;

	//std::cout << "gltf compute_ this->model->vertices.buffer.bufferData.size: " << this->model->vertices.buffer.bufferData.size << std::endl;
	
	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		storageInputBufferWrite
	};

	vkUpdateDescriptorSets(this->pCoreBase->devices.logical,
		static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

}

void Math_Compute::CreateCommandBuffers() {

	commandBuffers.resize(frame_draws);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = pCoreBase->commandPools.compute;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = frame_draws;

	if (vkAllocateCommandBuffers(pCoreBase->devices.logical, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to allocate compute command buffers!");
	}

}

void Math_Compute::RecordCommands(int frame) {

	vkCmdBindPipeline(this->commandBuffers[frame], VK_PIPELINE_BIND_POINT_COMPUTE, this->pipelineData.pipeline);

	vkCmdBindDescriptorSets(this->commandBuffers[frame],
		VK_PIPELINE_BIND_POINT_COMPUTE, this->pipelineData.pipelineLayout,
		0, 1, &this->pipelineData.descriptorSet, 0, nullptr);

	vkCmdDispatch(this->commandBuffers[frame], (ctCount / 256) + 1, 1, 1);

}

std::vector<Math_Compute::ComputeTestData> Math_Compute::RetrieveBufferData() {

	void* data = nullptr;

	vkMapMemory(this->pCoreBase->devices.logical, this->storageInputBuffer.bufferData.memory, 0, this->storageInputBuffer.bufferData.size, 0, &data);

	std::vector<ComputeTestData> tempTestData{};
	tempTestData.resize(ctCount);
	//std::vector<float> outputVector(this->count);

	memcpy(tempTestData.data(), data, (size_t)this->storageInputBuffer.bufferData.size);

	vkUnmapMemory(this->pCoreBase->devices.logical, this->storageInputBuffer.bufferData.memory);


	//std::cout << "retrieved buffer data count: " << outputVector.size() << std::endl;

	//for (int i = 0; i < outputVector.size(); i++) {
	//	std::cout << "retrieved buffer data[" << i << "]: " << outputVector[i] << std::endl;
	//}

	return tempTestData;

}
