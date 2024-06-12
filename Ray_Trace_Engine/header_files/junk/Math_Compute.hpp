#pragma once

#include <Tools.hpp>
#include <CoreBase.hpp>
#include <Shader.hpp>
#include <iomanip>

class Math_Compute {
public:
	// -- core pointer
	CoreBase* pCoreBase = nullptr;
	
	// -- shader
	Shader shader;

	struct ComputeTestData {
		glm::vec4 testVec4_a;
		glm::vec4 testVec4_b;
	};

	std::vector<ComputeTestData> ctData{};
	uint32_t ctCount = 0;

	// -- shader storage buffer
	vrt::Buffer storageInputBuffer;

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


	// -- ctor
	Math_Compute();

	// -- init ctor
	Math_Compute(CoreBase* coreBase);

	// -- destroy
	void Destroy_Math_Compute();

	// -- init function
	void Init_Math_Compute(CoreBase* coreBase);

	// -- create buffers
	void CreateBuffers();
	
	// -- create pipeline
	void CreatePipeline();

	// -- create descriptor sets
	void CreateDescriptorSets();
	
	// -- create command buffers
	void CreateCommandBuffers();

	// -- record commands
	void RecordCommands(int currentFrame);

	// -- retrieve buffer data
	std::vector<ComputeTestData> RetrieveBufferData();

};

