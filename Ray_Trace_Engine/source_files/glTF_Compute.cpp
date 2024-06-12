#include "glTF_Compute.hpp"

glTF_Compute::glTF_Compute() {

}

glTF_Compute::glTF_Compute(CoreBase* coreBase, vkglTF::Model* modelPtr) {

	Init_glTF_Compute(coreBase, modelPtr);

	CreateComputeBuffers();

	CreateComputePipeline();

	CreateDescriptorSets();

	CreateCommandBuffers();

}

void glTF_Compute::Init_glTF_Compute(CoreBase* coreBase, vkglTF::Model* modelPtr) {

	this->pCoreBase = coreBase;
	this->model = modelPtr;
	this->shader = Shader(coreBase);
}

void glTF_Compute::CreateComputeBuffers() {

	//input buffer
	this->storageInputBuffer.bufferData.bufferName = "gltf_compute_inputStorageBuffer_";
	this->storageInputBuffer.bufferData.bufferMemoryName = "gltf_compute_inputStorageBufferMemory_";
	
	this->pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&this->storageInputBuffer,
		static_cast<uint32_t>(this->model->modelVertexBuffer.size()) * sizeof(vkglTF::Vertex),
		nullptr);

	//output buffer
	this->storageOutputBuffer.bufferData.bufferName = "gltf_compute_outputStorageBuffer_";
	this->storageOutputBuffer.bufferData.bufferMemoryName = "gltf_compute_outputStorageBufferMemory_";
	
	this->pCoreBase->CreateBuffer(
		 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&this->storageOutputBuffer,
		static_cast<uint32_t>(this->model->modelVertexBuffer.size()) * sizeof(vkglTF::Vertex),
		nullptr);
	
	//copy from existing model vertex buffer
	//create temporary command buffer
	VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	
	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = model->vertices.buffer.bufferData.size;

	vkCmdCopyBuffer(cmdBuffer, model->vertices.buffer.bufferData.buffer, this->storageInputBuffer.bufferData.buffer, 1, &copyRegion);
	vkCmdCopyBuffer(cmdBuffer, model->vertices.buffer.bufferData.buffer, this->storageOutputBuffer.bufferData.buffer, 1, &copyRegion);

	//submit temporary command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//joint buffer
	std::vector<glm::mat4> transforms;

	//for (auto& nodes : this->model->linearNodes) {
	//	if (nodes->mesh) {
	//		if (nodes->skin) {
	//			std::cout << "\n\n**GLTF_COMPUTE joint buffer" <<  std::endl;
	//			for (int i = 0; i < nodes->skin->joints.size(); i++) {
	//				std::cout << "node["<< i <<"]name: " << nodes->skin->joints[i]->name << std::endl;
	//				transforms.push_back(nodes->transformMatrices[i]);
	//			}
	//		}
	//	}
	//}

	for (auto& nodes : this->model->linearNodes) {
		if (nodes->mesh) {
			if (nodes->skin) {
				std::cout << "\n\n**GLTF_COMPUTE joint buffer" << std::endl;
				for (int i = 0; i < nodes->skin->joints.size(); i++) {
					std::cout << "node[" << i << "]name: " << nodes->skin->joints[i]->name << std::endl;
					transforms.push_back(nodes->transformMatrices[i]);
				}
			}
		}
	}

	size_t jointBufferSize = transforms.size() * sizeof(glm::mat4);

	this->jointBuffer.bufferData.bufferName = "gltf_compute_jointStorageBuffer_";
	this->jointBuffer.bufferData.bufferMemoryName = "gltf_compute_jointStorageBufferMemory_";
	
	this->pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&this->jointBuffer,
		jointBufferSize,
		transforms.data());

}

void glTF_Compute::Destroy_glTF_Compute() {

	//descriptor pool
	vkDestroyDescriptorPool(this->pCoreBase->devices.logical, this->pipelineData.descriptorPool, nullptr);

	//shaders
	this->shader.DestroyShader();

	//pipeline and layout
	vkDestroyPipelineLayout(this->pCoreBase->devices.logical, this->pipelineData.pipelineLayout, nullptr);
	vkDestroyPipeline(this->pCoreBase->devices.logical, this->pipelineData.pipeline, nullptr);

	//descriptor set layout
	vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, this->pipelineData.descriptorSetLayout, nullptr);

	//storage buffers
	this->storageInputBuffer.destroy(this->pCoreBase->devices.logical);
	this->storageOutputBuffer.destroy(this->pCoreBase->devices.logical);
	this->jointBuffer.destroy(this->pCoreBase->devices.logical);

}

void glTF_Compute::CreateComputePipeline() {

	//descriptor binding
	VkDescriptorSetLayoutBinding inputStorageBufferLayoutBinding =
		vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
			0,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT,
			nullptr);

	VkDescriptorSetLayoutBinding outputStorageBufferLayoutBinding =
		vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
			1,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT,
			nullptr);

	VkDescriptorSetLayoutBinding jointStorageBufferLayoutBinding =
		vrt::Tools::VkInitializers::descriptorSetLayoutBinding(
			2,
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			1,
			VK_SHADER_STAGE_COMPUTE_BIT,
			nullptr);

	//array of bindings
	std::vector<VkDescriptorSetLayoutBinding> bindings({
	inputStorageBufferLayoutBinding,
	outputStorageBufferLayoutBinding,
	jointStorageBufferLayoutBinding
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
			"glTF_Compute_DescriptorSetLayout");

		//pipeline layout create info
		VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
		pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCreateInfo.setLayoutCount = 1;
		pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

		//create pipeline layout
		pCoreBase->add([this, &pipelineLayoutCreateInfo]()
			{return pCoreBase->objCreate.VKCreatePipelineLayout(&pipelineLayoutCreateInfo,
				nullptr, &pipelineData.pipelineLayout);}, "glTF_Compute_pipelineLayout");

		//project directory for loading shader modules
		std::filesystem::path projDirectory = std::filesystem::current_path();

		//Setup ray tracing shader groups
		VkPipelineShaderStageCreateInfo computeShaderStageCreateInfo;

		computeShaderStageCreateInfo = shader.loadShader(projDirectory.string() + "/shaders/compiled/gltf_compute.comp.spv", VK_SHADER_STAGE_COMPUTE_BIT,
			"gltf_compute_shader");
		VkComputePipelineCreateInfo computePipelineCreateInfo = {};
		computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		computePipelineCreateInfo.stage = computeShaderStageCreateInfo;
		computePipelineCreateInfo.layout = this->pipelineData.pipelineLayout;

		if (vkCreateComputePipelines(pCoreBase->devices.logical,
			VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &this->pipelineData.pipeline) != VK_SUCCESS) {
			throw std::invalid_argument("Failed to create gltf_compute pipeline!");
		}

}

void glTF_Compute::CreateCommandBuffers() {

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

void glTF_Compute::CreateDescriptorSets() {

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
	//storageInputBufferDescriptor.buffer = storageInputBuffer.bufferData.buffer;
	storageInputBufferDescriptor.buffer = this->model->vertices.buffer.bufferData.buffer;
	storageInputBufferDescriptor.offset = 0;
	storageInputBufferDescriptor.range = static_cast<uint32_t>(this->model->modelVertexBuffer.size()) * sizeof(vkglTF::Vertex);

	//storage input buffer descriptor write info
	VkWriteDescriptorSet storageInputBufferWrite{};
	storageInputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	storageInputBufferWrite.dstSet = pipelineData.descriptorSet;
	storageInputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageInputBufferWrite.dstBinding = 0;
	storageInputBufferWrite.pBufferInfo = &storageInputBufferDescriptor;
	storageInputBufferWrite.descriptorCount = 1;

	std::cout << "gltf compute_ this->model->vertices.buffer.bufferData.size: " << this->model->vertices.buffer.bufferData.size << std::endl;
	//storage output buffer descriptor info
	VkDescriptorBufferInfo storageOutputBufferDescriptor{};
	//storageOutputBufferDescriptor.buffer = this->model->vertices.buffer.bufferData.buffer;
	storageOutputBufferDescriptor.buffer = storageOutputBuffer.bufferData.buffer;
	storageOutputBufferDescriptor.offset = 0;
	storageOutputBufferDescriptor.range = static_cast<uint32_t>(this->model->modelVertexBuffer.size()) * sizeof(vkglTF::Vertex);
	
	//storage output buffer descriptor write info
	VkWriteDescriptorSet storageOutputBufferWrite{};
	storageOutputBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	storageOutputBufferWrite.dstSet = pipelineData.descriptorSet;
	storageOutputBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	storageOutputBufferWrite.dstBinding = 1;
	storageOutputBufferWrite.pBufferInfo = &storageOutputBufferDescriptor;
	storageOutputBufferWrite.descriptorCount = 1;
	
	//joint buffer descriptor info
	VkDescriptorBufferInfo jointBufferDescriptor{};
	jointBufferDescriptor.buffer = jointBuffer.bufferData.buffer;
	jointBufferDescriptor.offset = 0;
	jointBufferDescriptor.range = jointBuffer.bufferData.size;
	
	//joint buffer descriptor write info
	VkWriteDescriptorSet jointBufferWrite{};
	jointBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	jointBufferWrite.dstSet = pipelineData.descriptorSet;
	jointBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	jointBufferWrite.dstBinding = 2;
	jointBufferWrite.pBufferInfo = &jointBufferDescriptor;
	jointBufferWrite.descriptorCount = 1;


	std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
		storageInputBufferWrite,
		storageOutputBufferWrite,
		jointBufferWrite
	};

	vkUpdateDescriptorSets(this->pCoreBase->devices.logical,
		static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, VK_NULL_HANDLE);

}

void glTF_Compute::RecordComputeCommands(int frame) {


	vkCmdBindPipeline(this->commandBuffers[frame], VK_PIPELINE_BIND_POINT_COMPUTE, this->pipelineData.pipeline);

	vkCmdBindDescriptorSets(this->commandBuffers[frame],
		VK_PIPELINE_BIND_POINT_COMPUTE, this->pipelineData.pipelineLayout,
		0, 1, &this->pipelineData.descriptorSet, 0, nullptr);

	vkCmdDispatch(this->commandBuffers[frame], (static_cast<uint32_t>(this->model->modelVertexBuffer.size()) + 255) / 256, 1, 1);


}

std::vector<vkglTF::Vertex> glTF_Compute::RetrieveBufferData() {

	void* data = nullptr;

	vkMapMemory(
		this->pCoreBase->devices.logical, this->storageInputBuffer.bufferData.memory,
		0, static_cast<uint32_t>(this->model->modelVertexBuffer.size()) * sizeof(vkglTF::Vertex), 0, &data);

	std::vector<vkglTF::Vertex> tempVertex{};
	tempVertex.resize(this->model->modelVertexBuffer.size());
	//std::vector<float> outputVector(this->count);

	memcpy(tempVertex.data(), data, (size_t)static_cast<uint32_t>(this->model->modelVertexBuffer.size()) * sizeof(vkglTF::Vertex));

	vkUnmapMemory(this->pCoreBase->devices.logical, this->storageInputBuffer.bufferData.memory);

	return tempVertex;

}

void glTF_Compute::UpdateJointBuffer() {

	std::vector<glm::mat4> transforms;

	for (auto& nodes : this->model->linearNodes) {
		if (nodes->mesh) {
			if (nodes->skin) {
				for (int i = 0; i < nodes->skin->joints.size(); i++) {
					//std::cout << "node names: " << nodes->skin->joints[i]->name << std::endl;
					transforms.push_back(nodes->transformMatrices[i]);
				}
			}
		}
	}

	size_t jointBufferSize = transforms.size() * sizeof(glm::mat4);

	this->jointBuffer.copyTo(transforms.data(), jointBufferSize);

}
