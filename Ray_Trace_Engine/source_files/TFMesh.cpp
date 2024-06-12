#include "TFMesh.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

TFMesh::TFMesh() { }

TFMesh::TFMesh(CoreBase* coreBase, glm::mat4 matrix, std::string name) {
	InitTFMesh(coreBase, matrix, name);
}

void TFMesh::InitTFMesh(CoreBase* coreBase, glm::mat4 matrix, std::string name) {

	this->pCoreBase = coreBase;
	this->uniformBlock.matrix = matrix;
	this->meshName = name;

	CreateMeshUBO();

}

void TFMesh::CreateMeshUBO() {
	uniformBufferData.ubo.bufferData.bufferName = this->meshName + "_TFMesh_uboBuffer";
	uniformBufferData.ubo.bufferData.bufferMemoryName = this->meshName + "_TFMesh_uboBufferMemory";

	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&uniformBufferData.ubo,
		sizeof(uniformBlock),
		&uniformBlock) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create tfmesh ubo buffer");
	}

	uniformBufferData.descriptor.buffer = uniformBufferData.ubo.bufferData.buffer;
	uniformBufferData.descriptor.offset = 0;
	uniformBufferData.descriptor.range = sizeof(uniformBlock);
}

void TFMesh::Primitive::SetDimensions(glm::vec3 min, glm::vec3 max) {
	dimensions.min = min;
	dimensions.max = max;
	dimensions.size = max - min;
	dimensions.center = (min + max) / 2.0f;
	dimensions.radius = glm::distance(min, max) / 2.0f;
}

void TFMesh::Material::CreateDescriptorSet(CoreBase* coreBase,
 VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags) {

	//descriptor set allocate info
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = descriptorPool;
	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
	descriptorSetAllocInfo.descriptorSetCount = 1;

	//allocate
	if (vkAllocateDescriptorSets(coreBase->devices.logical, &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::invalid_argument("failed to allocate gltf model MATERIAL descriptor set");
	}

	//image descriptor vector
	std::vector<VkDescriptorImageInfo> imageDescriptors{};

	//write set vector
	std::vector<VkWriteDescriptorSet> writeDescriptorSets{};


	if (descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageBaseColor)) {

		imageDescriptors.push_back(baseColorTexture.descriptor);

		baseColorTexture.descriptor.imageLayout = baseColorTexture.imageLayout;
		baseColorTexture.descriptor.imageView = baseColorTexture.view;
		baseColorTexture.descriptor.sampler = baseColorTexture.sampler;

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
		writeDescriptorSet.pImageInfo = &baseColorTexture.descriptor;
		writeDescriptorSets.push_back(writeDescriptorSet);
	}

	//std::cout << "\n INSIDE gltf texture descriptor set test 3" << std::endl;

	if (normalTexture.index > -1 && descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageNormalMap)) {
		imageDescriptors.push_back(normalTexture.descriptor);

		normalTexture.descriptor.imageLayout = normalTexture.imageLayout;
		normalTexture.descriptor.imageView = normalTexture.view;
		normalTexture.descriptor.sampler = normalTexture.sampler;

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
		writeDescriptorSet.pImageInfo = &normalTexture.descriptor;
		writeDescriptorSets.push_back(writeDescriptorSet);
	}

	//std::cout << "\n INSIDE gltf texture descriptor set test 4" << std::endl;

	vkUpdateDescriptorSets(coreBase->devices.logical, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

	//std::cout << "\n INSIDE gltf texture descriptor set test 5" << std::endl;

}
