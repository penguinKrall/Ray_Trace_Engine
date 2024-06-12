#pragma once

#include <Tools.hpp>
#include <Shader.hpp>
#include <filesystem>

#include <Texture.hpp>

#include <RTObjects.hpp>

// -- Raytracing class
class Raytracing {

private:

	// -- init raytracing objects
	void InitRaytracing(CoreBase* coreBase);

public:

	// Descriptor set pool
	VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

	// -- shader class
	Shader shader;

	// -- pointer to core
	//@brief references device, command, extensions, swapchain
	CoreBase* pCoreBase = nullptr;

	// -- raytracing storage image struct
	struct StorageImage {
		VkDeviceMemory memory;
		VkImage image;
		VkImageView view;
		VkFormat format;
	};

	// -- uniform data for raytracing uniform buffer object
	struct UniformData {
		glm::mat4 viewInverse;
		glm::mat4 projInverse;
	};

	// -- all the buffers for this raytracing class
	struct Buffers {

		//// -- raytracing uniform buffer object
		vrt::Buffer ubo;
		 
		// -- instances
		vrt::Buffer instancesBuffer{};

		// -- raytracing scratch buffer
		vrt::Buffer scratchBuffer{};

		// -- raytracing scene geometry vertex buffer
		vrt::Buffer vertexBuffer = vrt::Buffer();

		// -- raytracing scene geometry index buffer
		vrt::Buffer indexBuffer = vrt::Buffer();

		// -- raytracing scene geometry transformation buffer
		vrt::Buffer transformBuffer;

		// -- raytracing ray generation shader binding table
		vrt::Buffer raygenShaderBindingTable;
		VkStridedDeviceAddressRegionKHR raygenStridedDeviceAddressRegion{};
		// -- raytracing miss shader binding table
		vrt::Buffer missShaderBindingTable;
		VkStridedDeviceAddressRegionKHR missStridedDeviceAddressRegion{};
		// -- raytracing hit shader binding table
		vrt::Buffer hitShaderBindingTable;
		VkStridedDeviceAddressRegionKHR hitStridedDeviceAddressRegion{};
	};

	// -- pipeline data
	struct PipelineData {
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;
	};
	
	// -- raytracing buffers
	Buffers buffers{};

	// -- raytracing storage image struct
	StorageImage storageImage{};

	// -- raytracing uniform buffer object data struct
	UniformData uniformData{};

	// -- raytracing pipeline data
	PipelineData pipelineData{};

	// -- raytracing scene geometry indices count
	uint32_t indexCount;

	// -- raytracing shader group create infos
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

	// -- acceleration structure data
	struct AccelerationStructure {
		VkAccelerationStructureKHR accelerationStructureKHR;
		uint64_t deviceAddress = 0;
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	//bool resized = false;
	//bool viewUpdated = false;

	// -- bottom level acceleration structure
	AccelerationStructure bottomLevelAS{};

	// -- top level acceleration structure
	AccelerationStructure topLevelAS{};

	//texture
	vrt::Texture texture;

	// -- default constructor
	Raytracing();

	// -- constructor that performs InitRaytracing member function
	Raytracing(CoreBase* coreBase);

	// -- create scratch buffer
	//@brief maps buffer/buffer memory name and handle
	void createScratchBuffer(vrt::Buffer* buffer, VkDeviceSize size, std::string name);

	// -- delete scratch buffer
	//@brief destroys buffer/buffer memory
	void deleteScratchBuffer(vrt::Buffer& buffer);

	// -- create acceleration structure buffer
	//@brief maps buffer/buffer memory name and handle
	//@param std::string bufferName
	void createAccelerationStructureBuffer(AccelerationStructure& accelerationStructure,
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, std::string bufferName);

	// -- create bottom level acceleration structure
	//@brief bottom level structure contains the scenes geometry
	void createBottomLevelAccelerationStructure();

	// -- create top level acceleration structure
	//@brief top level acceleration structure contains the scene's object instances
	void createTopLevelAccelerationStructure();

	// -- create storage image
	void createStorageImage();

	// -- update uniform buffers
	void updateUniformBuffers();

	// -- create uniform buffers
	void createUniformBuffer();

	// -- create raytracing pipelines
	void createRayTracingPipeline();

	// -- create shader binding table 
	void createShaderBindingTable();

	// -- create descriptor sets
	//create the descriptor sets used for the ray tracing dispatch
	void createDescriptorSets();

	// -- build command buffers
	//@brief command buffer generation
	void buildCommandBuffers();

	void rebuildCommandBuffers(uint32_t frame);

	// -- handle resize
	//@brief handle resized window
	void handleResize();

	// -- get buffer device address
	//@return pCoreBase->coreExtensions->vkGetBufferDeviceAddress()
	uint64_t getBufferDeviceAddress(VkBuffer buffer);

	// -- destroy raytracing
	//@brief destroys raytracing related objects
	void DestroyRaytracing();
};

