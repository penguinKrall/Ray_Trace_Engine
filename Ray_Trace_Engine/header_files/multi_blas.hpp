#pragma once

#include <CoreBase.hpp>
#include <Shader.hpp>
#include <VulkanglTFModel.h>
#include <glTF_Compute.hpp>
#include <RTObjects.hpp>

class multi_blas {
public:

	std::vector<vrt::RTObjects::GeometryNode> geometryNodeBuf;
	std::vector<vrt::RTObjects::GeometryIndex> geometryIndexBuf;

	// -- uniform data struct
	struct UniformData {
		glm::mat4 viewInverse = glm::mat4(1.0f);
		glm::mat4 projInverse = glm::mat4(1.0f);
		glm::vec4 lightPos = glm::vec4(0.0f);
	};

	//storage buffer data struct
	struct StorageBufferData {
		std::vector<int> reflectGeometryID;
	};

	// -- buffers
	struct Buffers {
		vrt::Buffer transformBuffer{};
		vrt::Buffer g_nodes_buffer{};
		vrt::Buffer g_nodes_indices{};
		vrt::Buffer blas_scratch{};
		vrt::Buffer second_blas_scratch{};
		vrt::Buffer tlas_scratch{};
		vrt::Buffer tlas_instancesBuffer;
		vrt::Buffer ubo;
	};

	// -- assets data struct
	struct Assets {

		//models
		std::vector<vkglTF::Model*> models;

		//animation model
		vkglTF::Model* animatedModel = nullptr;

		//static/scene model
		vkglTF::Model* helmetModel = nullptr;

		//static/scene model
		vkglTF::Model* reflectionSceneModel = nullptr;

		//static/scene model
		vkglTF::Model* sampleBuildingModel = nullptr;

	};

	// -- pipeline data struct
	struct PipelineData {
		VkPipeline pipeline{};
		VkPipelineLayout pipelineLayout{};
		VkDescriptorSet descriptorSet{};
		VkDescriptorSetLayout descriptorSetLayout{};
		VkDescriptorPool descriptorPool{};
	};

	// -- raytracing ray generation shader binding table
	vrt::Buffer raygenShaderBindingTable;
	VkStridedDeviceAddressRegionKHR raygenStridedDeviceAddressRegion{};

	// -- raytracing miss shader binding table
	vrt::Buffer missShaderBindingTable;
	VkStridedDeviceAddressRegionKHR missStridedDeviceAddressRegion{};

	// -- raytracing hit shader binding table
	vrt::Buffer hitShaderBindingTable;
	VkStridedDeviceAddressRegionKHR hitStridedDeviceAddressRegion{};

	// -- shader groups
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups{};

	// -- top level acceleration structure
	vrt::RTObjects::AccelerationStructure TLAS{};

	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- storage image
	vrt::RTObjects::StorageImage storageImage{};

	// -- uniform data
	UniformData uniformData{};

	// -- storage buffer data
	StorageBufferData storageBufferData{};

	// -- bottom level acceleration structures
	std::vector<vrt::RTObjects::BLASData> bottomLevelAccelerationStructures;

	//// -- BLAS data 
	//vrt::RTObjects::BLASData blasData{};
	//
	//// -- BLAS data 
	//vrt::RTObjects::BLASData secondBlasData{};
	//
	//// -- BLAS data 
	//vrt::RTObjects::BLASData thirdBlasData{};

	// -- TLAS data
	vrt::RTObjects::TLASData tlasData{};

	// -- Buffers
	Buffers buffers;

	// -- assets
	Assets assets{};

	// -- pipeline data
	PipelineData pipelineData{};

	// -- shader
	Shader shader;

	// -- compute
	glTF_Compute gltf_compute;

	// -- constructor
	multi_blas();

	// -- init constructor
	multi_blas(CoreBase* coreBase);

	// -- init class function
	void Init_multi_blas(CoreBase* coreBase);

	// -- load assets
	void LoadAssets();

	// -- create bottom level acceleration structures
	void CreateBottomLevelAccelerationStructures();

	// -- create top level acceleration structure
	void CreateTLAS();

	// -- create storage image
	void CreateStorageImage();

	// -- create uniform buffer
	void CreateUniformBuffer();

	// -- update uniform buffer
	void UpdateUniformBuffer(float deltaTime);

	// -- create ray tracing pipeline
	void CreateRayTracingPipeline();

	// -- create shader binding tables
	void CreateShaderBindingTable();

	// -- create descriptor set
	void CreateDescriptorSet();

	// -- setup buffer region addresses
	void SetupBufferRegionAddresses();

	// -- build command buffers
	void BuildCommandBuffers();

	// -- rebuild command buffers
	void RebuildCommandBuffers(int frame);

	// -- update BLAS
	void UpdateBLAS();

	// -- update TLAS
	void UpdateTLAS();

	// -- pre transform animation model vertices
	void PreTransformModel();

	// -- create geometry nodes buffer
	void CreateGeometryNodesBuffer();

	// -- destroy class objects
	void Destroy_multi_blas();


};

