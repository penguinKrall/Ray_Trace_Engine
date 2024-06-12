#pragma once

#include <CoreBase.hpp>
#include <ShaderBindingTable.hpp>
#include <RTObjects.hpp>
//#include <TFModel.hpp>
#include <Shader.hpp>
#include <Texture.hpp>
//#include <glTF_model.hpp>
#include <VulkanglTFModel.h>
#include <glTF_Compute.hpp>

#define VK_GLTF_MATERIAL_IDS

class  glTFAnimation {

public:

	struct GeometryNode {
		uint64_t vertexBufferDeviceAddress = 0;
		uint64_t indexBufferDeviceAddress = 0;
		int textureIndexBaseColor = 0;
		int textureIndexOcclusion = 0;
	};

	// -- glTFAnimation buffers
	struct Buffers {
		vrt::Buffer transformBuffer{};
		vrt::Buffer geometryNodesBuffer{};
		vrt::Buffer blas_scratch{};
		vrt::Buffer tlas_scratch{};
		vrt::Buffer tlas_instancesBuffer;
	};

	// -- uniform data struct
	struct UniformData {
		glm::mat4 viewInverse = glm::mat4(1.0f);
		glm::mat4 projInverse = glm::mat4(1.0f);
		glm::vec4 lightPos = glm::vec4(0.0f);
		//glm::mat4 testMat = glm::mat4(1.0f);
		uint32_t frame{ 0 };

	};

	// -- BLAS data struct
	struct BLASData {
		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
		VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
		uint32_t maxPrimCount{ 0 };
		std::vector<uint32_t> maxPrimitiveCounts{};
		std::vector<VkAccelerationStructureGeometryKHR> geometries{};
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos{};
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos{};
		std::vector<GeometryNode> geometryNodes{};
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
	};

	// -- TLAS data struct
	struct TLASData {
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos;
		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
	};

	// -- BLAS data 
	BLASData blasData{};

	// -- TLAS data
	TLASData tlasData{};

	//test ubo
	struct TestUniformData {
		glm::mat4 testMat = glm::mat4(1.0f);
	};

	TestUniformData testUniformData{};

	vrt::Buffer testUBO;

	void CreateTestUBO();

	//float rotationTime = 0.0f;

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

	// -- pipeline data
	PipelineData pipelineData{};

	// -- uniform data
	UniformData uniformData{};

	// -- uniform buffer
	vrt::Buffer ubo;

	// -- storage image
	vrt::RTObjects::StorageImage storageImage{};

	// -- buffers
	Buffers buffers{};

	// -- shader
	Shader shader;

	// -- compute
	glTF_Compute gltf_compute;

	// -- model
	//TFModel tfModel;

	// -- the new model -- // 
	//glTF_model gltf_model;

	//new new model
	vkglTF::Model* model;

	// -- memory property flags
	VkMemoryPropertyFlags memoryPropertyFlags = 0;

	// -- bottom level acceleration structure
	vrt::RTObjects::AccelerationStructure BLAS{};

	// -- top level acceleration structure
	vrt::RTObjects::AccelerationStructure TLAS{};

	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- default constructor
	glTFAnimation();

	// -- init constructor
	glTFAnimation(CoreBase* coreBase);

	// -- destroy
	void DestroyglTFAnimation();

	// -- update uniform buffer
	void UpdateUniformBuffer(float deltaTime);

	// -- rebuild command buffers
	void RebuildCommandBuffers(int frame);

	// -- init function
	void InitglTFAnimation(CoreBase* coreBase);

	// -- load assets
	void LoadAssets();

	// -- create bottom level acceleration structure
	void CreateBLAS();

	// -- create top level acceleration structure
	void CreateTLAS();

	// -- recreate acceleration structures
	void RecreateAccelerationStructures();

	// -- create storage image
	void CreateStorageImage();

	// -- create uniform buffers
	void CreateUniformBuffer();

	// -- create raytracing pipeline
	void CreateRayTracingPipeline();

	// -- create shader binding tables
	void CreateShaderBindingTable();

	// -- create descriptor set
	void CreateDescriptorSet();

	// -- setup buffer region addresses
	void SetupBufferRegionAddresses();

	// -- build command buffers
	void BuildCommandBuffers();

	// -- update vertex buffers
	void UpdateVertexBuffers();

	// -- update BLAS
	void UpdateBLAS(float timer);

	// -- update TLAS
	void UpdateTLAS();

};