#pragma once

#include <VulkanglTFModel.h>
#include <Tools.hpp>
#include <Shader.hpp>
#include <filesystem>

class glTF_Compute {
public:

	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- shader
	Shader shader;

	// -- gltf_model
	//this is where the vertex buffers are that will be used by the compute shader and subsequently the ray tracing shaders
	vkglTF::Model* model = nullptr;

	// -- shader storage buffer
	vrt::Buffer storageInputBuffer;
	vrt::Buffer storageOutputBuffer;

	// -- uniform buffer
	vrt::Buffer jointBuffer;

	// -- pipeline data struct
	struct PipelineData {
		VkDescriptorPool descriptorPool{};
		VkDescriptorSetLayout descriptorSetLayout{};
		VkDescriptorSet descriptorSet{};
		VkPipeline pipeline{};
		VkPipelineLayout pipelineLayout{};
	};

	// -- command buffers
	std::vector<VkCommandBuffer> commandBuffers;

	// -- pipeline data
	PipelineData pipelineData{};

	// -- default constructor
	glTF_Compute();

	// -- init constructor
	glTF_Compute(CoreBase* coreBase, vkglTF::Model* modelPtr);

	// -- init func
	void Init_glTF_Compute(CoreBase* coreBase, vkglTF::Model* modelPtr);

	// -- create buffers
	void CreateComputeBuffers();

	// -- create pipeline
	void CreateComputePipeline();

	// -- create descriptor sets
	void CreateDescriptorSets();

	// -- create command buffers
	void CreateCommandBuffers();

	// -- update joint buffers
	void UpdateJointBuffer();

	// -- record compute commands
	void RecordComputeCommands(int frame);

	// -- retrieve buffer data
	std::vector<vkglTF::Vertex> RetrieveBufferData();

	// -- destroy
	void Destroy_glTF_Compute();
};

