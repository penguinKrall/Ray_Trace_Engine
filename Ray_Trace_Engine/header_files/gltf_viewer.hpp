#pragma once

#include <CoreBase.hpp>
#include <Shader.hpp>
#include <gltf_viewer_model.hpp>
#include <gltf_viewer_compute.hpp>
#include <gltf_viewer_rt_utils.hpp>
#include <Texture.hpp>

class gltf_viewer {
public:

	std::vector<gltf_viewer_rt_utils::GeometryNode> geometryNodeBuf;
	std::vector<gltf_viewer_rt_utils::GeometryIndex> geometryIndexBuf;

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
		std::vector<GVM::Model*> models;

		//animation model
		GVM::Model* animatedModel;

		////static/scene model
		//GVM::Model helmetModel;
		//
		////static/scene model
		//GVM::Model reflectionSceneModel;
		//
		////static/scene model
		GVM::Model* testScene;
		//
		////static/scene model
		//GVM::Model directionCube;
		//
		////building glass model
		GVM::Model* waterSurface;
		//
		//colored glass texture
		vrt::Texture coloredGlassTexture;
		//
		////gondola model
		//GVM::Model* gondola;

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
	gltf_viewer_rt_utils::AccelerationStructure TLAS{};

	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- storage image
	gltf_viewer_rt_utils::StorageImage storageImage{};

	// -- uniform data
	UniformData uniformData{};

	// -- storage buffer data
	StorageBufferData storageBufferData{};

	// -- bottom level acceleration structures
	std::vector<gltf_viewer_rt_utils::BLASData> bottomLevelAccelerationStructures;

	//// -- BLAS data 
	//gltf_viewer_rt_utils::BLASData blasData{};
	//
	//// -- BLAS data 
	//gltf_viewer_rt_utils::BLASData secondBlasData{};
	//
	//// -- BLAS data 
	//gltf_viewer_rt_utils::BLASData thirdBlasData{};

	// -- TLAS data
	gltf_viewer_rt_utils::TLASData tlasData{};

	// -- Buffers
	Buffers buffers;

	// -- assets
	Assets assets{};

	// -- pipeline data
	PipelineData pipelineData{};

	// -- shader
	Shader shader;

	// -- compute
	gltf_viewer_compute gltfCompute;

	// -- constructor
	gltf_viewer();

	// -- init constructor
	gltf_viewer(CoreBase* coreBase);

	// -- init class function
	void Init_gltf_viewer(CoreBase* coreBase);

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

	// -- update descriptor set
	void UpdateDescriptorSet();

	// -- handle window resize 
	void HandleResize();

	// -- destroy class objects
	void Destroy_gltf_viewer();
};

