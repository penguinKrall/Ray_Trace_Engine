#pragma once

#include <CoreBase.hpp>
//#include <Texture.hpp>
#include <TFMaterial.hpp>

// -- class decl to hold mesh data
// -- fk some other peoples data structures and fk mine but i know what this sh1t is
class TFMesh {
public:

	// -- descriptor binding flags
	enum class DescriptorBindingFlags {
		ImageBaseColor = 0x00000001,
		ImageNormalMap = 0x00000002
	};

	//// -- vertex component
	////@brief  glTF default vertex layout with easy Vulkan mapping functions
	//enum class VertexComponent { Position, Normal, UV, Color, Tangent, Joint0, Weight0 };

	//// -- vertex struct
	//struct Vertex {
	//	glm::vec3 pos;
	//	glm::vec3 normal;
	//	glm::vec2 uv;
	//	glm::vec4 color;
	//	glm::vec4 joint0;
	//	glm::vec4 weight0;
	//	glm::vec4 tangent;
	//	//static VkVertexInputBindingDescription vertexInputBindingDescription;
	//	//static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
	//	//static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
	//	//static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);
	//	//static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
	//	//static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components);
	//	///** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
	//	//static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(const std::vector<VertexComponent> components);
	//};

	// -- dimensions struct
	struct Dimensions {
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size;
		glm::vec3 center;
		float radius;
	};

	// -- material struct 
	struct Material {

		enum class AlphaMode {
			ALPHAMODE_OPAQUE = 0,
			ALPHAMODE_MASK = 1,
			ALPHAMODE_BLEND = 2
		};

		AlphaMode alphaMode = AlphaMode::ALPHAMODE_OPAQUE;

		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		vrt::Texture baseColorTexture;
		vrt::Texture metallicRoughnessTexture;
		vrt::Texture normalTexture;
		vrt::Texture occlusionTexture;
		vrt::Texture emissiveTexture;
		vrt::Texture specularGlossinessTexture;
		vrt::Texture diffuseTexture;
		vrt::Texture emptyTexture;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		// -- create descriptor set
		//@brief material class 
		void CreateDescriptorSet(CoreBase* coreBase, VkDescriptorPool descriptorPool,
			VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags);
	};

	// -- primitive struct
	struct Primitive {

		// -- material class decl.
		//vrt::TFMaterial material;
		Material material;

		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t firstVertex;
		uint32_t vertexCount;
		// -- dimensions struct decl.
		Dimensions dimensions{};

		void SetDimensions(glm::vec3 min, glm::vec3 max);
	};

	// -- uniform buffer data
	struct UniformBufferData {
		vrt::Buffer ubo;
		VkDescriptorBufferInfo descriptor;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
	};

	// -- uniform block
	struct UniformBlock {
		glm::mat4 matrix;
		glm::mat4 jointMatrix[64]{};
		float jointcount{ 0 };
	};

	// -- mesh name
	std::string meshName;

	// -- primitives struct decl.
	std::vector<Primitive> primitives;

	// -- uniform buffer object
	UniformBufferData uniformBufferData{};

	// -- uniform buffer data
	UniformBlock uniformBlock{};

	// -- core pointer
	CoreBase* pCoreBase = nullptr;


	// -- default constructor
	TFMesh();

	// -- init constructor
	TFMesh(CoreBase* coreBase, glm::mat4 matrix, std::string name);

	// -- init func
	void InitTFMesh(CoreBase* coreBase, glm::mat4 matrix, std::string name);

	// -- create mesh ubo 
	void CreateMeshUBO();

};

