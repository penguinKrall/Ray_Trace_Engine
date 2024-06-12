#pragma once

#include <CoreBase.hpp>
#include <ShaderBindingTable.hpp>
//#include <TFModel.hpp>
#include <RTObjects.hpp>
#include <TFModel.hpp>
#include <Shader.hpp>

// --  glTFShadows
class glTFShadows{

public:

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

	Buffers buffers{};

	int indexCount = 0;
	int vertexCount = 0;

	//vrt::TFModel tfModel;

	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- uniform data struct
	struct UniformData {
		glm::mat4 viewInverse;
		glm::mat4 projInverse;
		glm::vec4 lightPos;
		int32_t vertexSize;
	};

	// -- acceleration structure data struct
	struct AccelerationStructure {
		VkAccelerationStructureKHR accelerationStructureKHR;
		uint64_t deviceAddress = 0;
		VkDeviceMemory memory;
		VkBuffer buffer;
	};

	// -- shader binding tables data struct
	struct ShaderBindingTables {
		ShaderBindingTable ray;
		ShaderBindingTable miss;
		ShaderBindingTable hit;
	};

	// -- pipeline data struct
	struct PipelineData {
		VkPipeline pipeline;
		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;
		VkDescriptorSetLayout descriptorSetLayout;
	};

	// -- descriptor pool
	VkDescriptorPool descriptorPool{};


	// -- shader
	Shader shader;

	// -- raytracing shader group create infos
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

	// -- raytracing ray generation shader binding table
	vrt::Buffer raygenShaderBindingTable;
	VkStridedDeviceAddressRegionKHR raygenStridedDeviceAddressRegion{};

	// -- raytracing miss shader binding table
	vrt::Buffer missShaderBindingTable;
	VkStridedDeviceAddressRegionKHR missStridedDeviceAddressRegion{};

	// -- raytracing hit shader binding table
	vrt::Buffer hitShaderBindingTable;
	VkStridedDeviceAddressRegionKHR hitStridedDeviceAddressRegion{};

	// -- uniform buffer data
	UniformData uniformData{};

	// -- uniform buffer object
	vrt::Buffer ubo;

	// -- bottom level acceleration structure
	AccelerationStructure bottomLevelAS{};

	// -- top level acceleration structure
	AccelerationStructure topLevelAS{};


	struct StorageImage {
		VkDeviceMemory memory;
		VkImage image;
		VkImageView view;
		VkFormat format;
	};

	// -- storage image
	StorageImage storageImage{};

	// -- create storage image
	void CreateStorageImage();

	// -- glTFShadows shader binding tables member
	ShaderBindingTables shaderBindingTables{};

	// -- pipeline data member
	PipelineData pipelineData{};

	// TFNode class
	TFModel tfModel{};

	// -- ctor
	glTFShadows();
	
	// -- init ctor
	glTFShadows(CoreBase* coreBase);

	// -- init func called by init ctor
	void InitglTFShadows(CoreBase* coreBase);

	// -- create acceleration structure buffer
	//void createAccelerationStructureBuffer(AccelerationStructure& accelerationStructure,
	//	VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo, std::string bufferName);

	// -- create bottom level acceleration structure
	void CreateBLAS();
	
	// -- create top level acceleration structure
	void CreateTLAS();

	// -- create uniform buffer
	void CreateUniformBuffer();

	// -- update uniform buffer
	void UpdateUniformBuffer(float deltaTime);

	// -- create ray tracing pipeline
	void CreateRayTracingPipeline();

	// -- create shader binding tables
	void CreateShaderBindingTables();

	// -- create descriptor sets
	void CreateDescriptorSets();

	// -- setup buffer region addresses
	void SetupBufferRegionAddresses();

	// -- build command buffers
	void BuildCommandBuffers();

	// -- rebuild command buffers
	void RebuildCommandBuffers(uint32_t frame);

	// -- get buffer device address
	uint64_t GetBufferDeviceAddress(VkBuffer buffer) {
		VkBufferDeviceAddressInfoKHR bufferDeviceAI{};
		bufferDeviceAI.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		bufferDeviceAI.buffer = buffer;
		return pCoreBase->coreExtensions->vkGetBufferDeviceAddressKHR(pCoreBase->devices.logical, &bufferDeviceAI);
	}

	// -- destroy
	void DestroyglTFShadows();

};

