#pragma once

#include <TFNode.hpp>
#include <Texture.hpp>
//#include <vrtTFSkin.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class TFModel {

public:

	glm::mat4 modelTransformMatrix = glm::mat4(1.0f);

	// -- vertex component
	//@brief  glTF default vertex layout with easy Vulkan mapping functions
	enum class VertexComponent { Position, Normal, UV, Color, Tangent, Joint0, Weight0 };

	// -- vertex struct
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec4 color;
		glm::vec4 joint0;
		glm::vec4 weight0;
		glm::vec4 tangent;
		//static VkVertexInputBindingDescription vertexInputBindingDescription;
		//static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		//static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
		//static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);
		//static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
		//static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components);
		///** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
		//static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(const std::vector<VertexComponent> components);
	};

	// -- vertices data struct
	struct Vertices {
		int count;
		vrt::Buffer verticesBuffer{};
	};

	// -- indices data struct
	struct Indices {
		int count;
		vrt::Buffer indicesBuffer{};
	};

	// -- vertices
	Vertices vertices{};

	// -- indices
	Indices indices{};

	// -- descriptor binding flags
	enum class DescriptorBindingFlags {
		ImageBaseColor = 0x00000001,
		ImageNormalMap = 0x00000002
	};

	//model descriptor binding flags
	uint32_t descriptorBindingFlags = static_cast<uint32_t>(TFModel::DescriptorBindingFlags::ImageBaseColor);
	
	// -- file loading flags
	enum class FileLoadingFlags {
		None = 0x00000000,
		PreTransformVertices = 0x00000001,
		PreMultiplyVertexColors = 0x00000002,
		FlipY = 0x00000004,
		DontLoadImages = 0x00000008
	};

	// -- render flags
	enum class RenderFlags {
		BindImages = 0x00000001,
		RenderOpaqueNodes = 0x00000002,
		RenderAlphaMaskedNodes = 0x00000004,
		RenderAlphaBlendedNodes = 0x00000008
	};

	// -- descriptor data struct
	struct DescriptorData {
	VkDescriptorSetLayout descriptorSetLayoutImage;
	VkDescriptorSetLayout descriptorSetLayoutUbo;
	VkMemoryPropertyFlags memoryPropertyFlags;
	uint32_t descriptorBindingFlags;
	};

	// -- animation channel struct  
	struct AnimationChannel {
		enum PathType { TRANSLATION, ROTATION, SCALE };
		std::string path;
		TFNode* node = nullptr;
		uint32_t samplerIndex;
	};

	// -- animation sampler struct 
	struct AnimationSampler {
		enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
		std::string interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	// -- animation struct
	struct Animation {
		int testInt = 0;
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float currentTime = 0.0f;
	};

	//flags
	bool metallicRoughnessWorkflow = true;
	bool buffersBound = false;

	// -- descriptor pool
	VkDescriptorPool descriptorPool;

	//--descriptor set layouts
	VkDescriptorSetLayout descriptorSetLayoutImage = VK_NULL_HANDLE;
	VkDescriptorSetLayout descriptorSetLayoutUbo = VK_NULL_HANDLE;

	// -- file path
	std::string path;

	// -- animations
	std::vector<Animation> animations;

	// -- nodes
	std::vector<TFNode> nodes;

	// -- linear nodes
	std::vector<TFNode> linearNodes;

	// -- skins
	std::vector<TFNode::Skin> skins{};

	// -- textures
	std::vector<vrt::Texture> textures;

	// -- material data
	//Material material{};
	//std::vector<Material> materials;
	std::vector<TFMesh::Material> materials;
	// -- descriptor data
	DescriptorData descriptor{};

	// -- core pointer
	CoreBase* pCoreBase = nullptr;
	
	// -- ctor
	TFModel();

	// -- init ctor
	explicit TFModel(CoreBase* coreBase, std::string filePath);

	// -- init func
	void InitTFModel(CoreBase* coreBase, std::string filePath);

	// -- load model from file
	void LoadFromFile(std::string filename, CoreBase* coreBase, VkQueue transferQueue, uint32_t fileLoadingFlags, float scale);

	// -- load images
	void LoadImages(tinygltf::Model& model, VkQueue transferQueue);

	// -- get texture
	vrt::Texture GetTexture(uint32_t index);

	// -- load materials
	void LoadMaterials(tinygltf::Model& model);

	// -- load node
	void LoadNode(TFNode* parent, const tinygltf::Node& node, uint32_t nodeIndex,
		const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale);

	// -- load skins
	void LoadSkins(tinygltf::Model& model);

	// -- find node
	TFNode* FindNode(TFNode* parent, uint32_t index);

	// -- node from index
	TFNode* NodeFromIndex(uint32_t index);

	// -- load animations
	void LoadAnimations(tinygltf::Model model);

	struct Dimensions {
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
		glm::vec3 size;
		glm::vec3 center;
		float radius;
	} dimensions;

	//vectors for indices and vertices
	std::vector<uint32_t> indexBuffer;
	std::vector<Vertex> vertexBuffer;

	// -- get node dimensions
	void GetNodeDimensions(TFNode* node, glm::vec3& min, glm::vec3& max);

	// -- get scene dimensions
	void GetSceneDimensions();

	// -- prepare node descriptor
	void PrepareNodeDescriptor(TFNode* node, VkDescriptorSetLayout descriptorSetLayout);

	// -- update animation
	void UpdateAnimation(uint32_t index, float time);

	// -- update joints
	void UpdateJoints(TFNode* node);

	// -- get node matrix
	glm::mat4 GetNodeMatrix(TFNode* node)
	{
		glm::mat4              nodeMatrix = node->LocalMatrix();
		TFNode* currentParent = node->parentNode;
		while (currentParent)
		{
			nodeMatrix = currentParent->LocalMatrix() * nodeMatrix;
			currentParent = currentParent->parentNode;
		}
		return nodeMatrix;
	}

	// -- update vertices and indices
	void UpdateVertexIndex(glm::mat4 transform);

	// -- destroy
	void DestroyModel();
};
