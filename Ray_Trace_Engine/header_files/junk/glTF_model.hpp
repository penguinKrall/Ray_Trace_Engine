#pragma once

#include <CoreBase.hpp>
#include <RTObjects.hpp>
#include <Texture.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tiny_gltf.h>

class glTF_model {

public:

	// -- descriptor binding flags
	enum class DescriptorBindingFlags {
		ImageBaseColor = 0x00000001,
		ImageNormalMap = 0x00000002
	};

	//model descriptor binding flags
	uint32_t descriptorBindingFlags = static_cast<uint32_t>(glTF_model::DescriptorBindingFlags::ImageBaseColor);

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

	// -- file path
	std::string path;

	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- device pointer
	VkDevice* pDevice = nullptr;

	// -- vertices struct
	struct Vertices {
		int count;
		vrt::Buffer buffer;
	} vertices;

	// -- indices struct
	struct Indices {
		int            count;
		vrt::Buffer buffer;
	} indices;

	struct Node;

	struct Material {
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		uint32_t  baseColorTextureIndex;
	};

	struct Image {
		vrt::Texture  texture;
		VkDescriptorSet descriptorSet;
	};

	struct Texture {
		int32_t imageIndex;
	};

	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		int32_t  materialIndex;
	};

	struct Mesh {
		std::vector<Primitive> primitives;
	};

	struct Node {
		Node* parent;
		uint32_t            index;
		std::vector<Node*> children;
		Mesh                mesh;
		glm::vec3           translation{};
		glm::vec3           scale{ 1.0f };
		glm::quat           rotation{};
		int32_t             skin = -1;
		glm::mat4           matrix;
		glm::mat4           GetLocalMatrix();
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec4 color;
		glm::vec4 joint0;
		glm::vec4 weight0;
		glm::vec4 tangent;
	};

	/*
		Skin structure
	*/

	struct Skin {
		std::string            name;
		Node* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<Node*>    joints;
		vrt::Buffer            ssbo;
		VkDescriptorSet        descriptorSet;
	};

	/*
		Animation related structures
	*/

	struct AnimationSampler {
		std::string            interpolation;
		std::vector<float>     inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	struct AnimationChannel {
		std::string path;
		Node* node;
		uint32_t    samplerIndex;
	};

	struct Animation {
		std::string                   name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float                         start = std::numeric_limits<float>::max();
		float                         end = std::numeric_limits<float>::min();
		float                         currentTime = 0.0f;
	};

	std::vector<Image>     images;
	std::vector<Texture>   textures;
	std::vector<Material>  materials;
	std::vector<Node*>		 nodes;
	std::vector<Node*>		 linearNodes;
	std::vector<Skin>      skins;
	std::vector<Animation> animations;

	uint32_t activeAnimation = 0;

	// -- indices
	std::vector<uint32_t> indexBuffer;

	// -- vertices
	std::vector<glTF_model::Vertex> vertexBuffer;

	// -- default constructor
	glTF_model();

	// -- init constructor
	glTF_model(CoreBase* coreBase, std::string fileName);

	// -- init function
	void Init_glTF_model(CoreBase* coreBase, std::string fileName);

	// -- load gltf file
	void LoadglTFFile(std::string filename, VkQueue transferQueue, uint32_t fileLoadingFlags, float scale);

	// -- load images
	void LoadImages(tinygltf::Model& input, VkQueue transferQueue);

	// -- load materials
	void LoadMaterials(tinygltf::Model& input);
	
	// -- load textures
	void LoadTextures(tinygltf::Model& input);

	// -- load node
	void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input,
		glTF_model::Node* parent, uint32_t nodeIndex, std::vector<uint32_t>& indexBuffer, std::vector<glTF_model::Vertex>& vertexBuffer);

	// -- load skins
	void LoadSkins(tinygltf::Model& input);

	// -- find node
	glTF_model::Node* FindNode(glTF_model::Node* parent, uint32_t index);

	// -- node from index
	glTF_model::Node* NodeFromIndex(uint32_t index);

	// -- load animations
	void LoadAnimations(tinygltf::Model& input);

	// -- get node matrix
	glm::mat4 GetNodeMatrix(glTF_model::Node* node);

	// -- update joints
	void UpdateJoints(glTF_model::Node* node);

	// -- destroy
	void Destroy_glTF_model();

};