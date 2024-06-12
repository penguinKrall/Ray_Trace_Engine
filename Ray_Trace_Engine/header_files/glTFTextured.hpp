#pragma once

#include <CoreBase.hpp>
#include <ShaderBindingTable.hpp>
//#include <TFModel.hpp>
#include <RTObjects.hpp>
#include <TFModel.hpp>
#include <Shader.hpp>

class glTFTextured {
public:

	struct GeometryNode {
		uint64_t vertexBufferDeviceAddress = 0;
		uint64_t indexBufferDeviceAddress = 0;
		int32_t textureIndexBaseColor = 0;
		int32_t textureIndexOcclusion = 0;
	};

	// -- glTFTextured buffers
	struct Buffers {
		vrt::Buffer transformBuffer{};
		vrt::Buffer geometryNodesBuffer{};
	};

	// -- uniform data struct
	struct UniformData {
		glm::mat4 viewInverse = glm::mat4(1.0f);
		glm::mat4 projInverse = glm::mat4(1.0f);
		glm::vec4 lightPos = glm::vec4(7.0f);
		glm::mat4 testMat = glm::mat4(1.0f);
		float frame = 0;

	};

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

	// -- model
	TFModel tfModel;

	// -- memory property flags
	VkMemoryPropertyFlags memoryPropertyFlags = 0;

	// -- bottom level acceleration structure
	vrt::RTObjects::AccelerationStructure BLAS{};

	// -- top level acceleration structure
	vrt::RTObjects::AccelerationStructure TLAS{};

	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- default constructor
	glTFTextured();

	// -- init constructor
	glTFTextured(CoreBase* coreBase);

	// -- destroy
	void DestroyglTFTextured();

	// -- update uniform buffer
	void UpdateUniformBuffer(float deltaTime);

	// -- rebuild command buffers
	void RebuildCommandBuffers(int frame);

	// -- init function
	void InitglTFTextured(CoreBase* coreBase);

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

	//--random test
	void RandomTest() {
		std::vector<glm::mat4> transformMatrices{};
		for (auto node : this->tfModel.linearNodes) {
			if (node.skin != nullptr) {
				std::cout << "node has mesh" << std::endl;
				std::cout << "node.name: " << node.name << std::endl;
				std::cout << "node.mesh.primitives.size(): " << node.mesh.primitives.size() << std::endl;
				for (int i = 0; i < node.mesh.primitives.size(); i++) {
					std::cout << "test" << std::endl;
					if (node.mesh.primitives[i].indexCount > 0) {
						std::cout << "primitive count > 0" << std::endl;
						glm::mat4 transformMatrix = node.GetMatrix();
						transformMatrices.push_back(transformMatrix);
						std::cout << "Transform Matrix:" << std::endl;
						for (int row = 0; row < 4; ++row) {
							for (int col = 0; col < 4; ++col) {
								std::cout << transformMatrix[row][col] << " ";
							}
							std::cout << std::endl;
						}
					}
				}
			}
		}
	}
};