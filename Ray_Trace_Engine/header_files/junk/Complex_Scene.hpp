#pragma once

#include <CoreBase.hpp>
#include <Shader.hpp>
#include <VulkanglTFModel.h>
#include <glTF_Compute.hpp>
#include <RTObjects.hpp>

class Complex_Scene {
public:

	struct GeometryNode {
		uint64_t vertexBufferDeviceAddress = 0;
		uint64_t indexBufferDeviceAddress = 0;
		int textureIndexBaseColor = 0;
		int textureIndexOcclusion = 0;
	};

	std::vector<GeometryNode> geometryNodes{};

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

	// -- buffers
	struct Buffers {
		vrt::Buffer transformBuffer{};
		vrt::Buffer geometryNodesBuffer{};
		vrt::Buffer blas_scratch{};
		vrt::Buffer tlas_scratch{};
		vrt::Buffer tlas_instancesBuffer;
		vrt::Buffer ubo;
		vrt::Buffer ssbo;
	};

	// -- assets data struct
	struct Assets {
		//animation model
		vkglTF::Model* animatedModel = nullptr;

		//static/scene model
		vkglTF::Model* sceneModel = nullptr;

		//static/scene model
		vkglTF::Model* staticModel = nullptr;
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

	// -- bottom level acceleration structure
	vrt::RTObjects::AccelerationStructure BLAS{};

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

	// -- BLAS data 
	BLASData blasData{};

	// -- TLAS data
	TLASData tlasData{};

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
	Complex_Scene();

	// -- init constructor
	Complex_Scene(CoreBase* coreBase);

	// -- init class function
	void Init_Complex_Scene(CoreBase* coreBase);

	// -- load assets
	void LoadAssets();

	// -- create bottom level acceleration structure
	void CreateBLAS();

	// -- create top level acceleration structure
	void CreateTLAS();

	// -- create storage image
	void CreateStorageImage();

	// -- create storage buffer
	void CreateStorageBuffer();

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

	// -- update vertex buffers
	void UpdateVertexBuffers();

	// -- pre transform animation model vertices
	void PreTransformModel();

	// -- destroy class objects
	void Destroy_Complex_Scene();


};

