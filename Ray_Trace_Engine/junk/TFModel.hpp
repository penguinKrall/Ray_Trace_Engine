#pragma once

#include <Tools.hpp>
#include <CoreBase.hpp>
#include <Texture.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <TFMesh.hpp>

namespace vrt {

	// -- node
	//@brief gltf node decl.
	struct Node;

	// -- descriptor binding flags
	enum DescriptorBindingFlags {
		ImageBaseColor = 0x00000001,
		ImageNormalMap = 0x00000002
	};

	extern VkDescriptorSetLayout descriptorSetLayoutImage;
	extern VkDescriptorSetLayout descriptorSetLayoutUbo;
	extern VkMemoryPropertyFlags memoryPropertyFlags;
	extern uint32_t descriptorBindingFlags;

	// -- material struct 
	struct Material {
		CoreBase* pCoreBase = nullptr;
		enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
		AlphaMode alphaMode = ALPHAMODE_OPAQUE;
		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);
		vrt::Texture* baseColorTexture = nullptr;
		vrt::Texture* metallicRoughnessTexture = nullptr;
		vrt::Texture* normalTexture = nullptr;
		vrt::Texture* occlusionTexture = nullptr;
		vrt::Texture* emissiveTexture = nullptr;

		vrt::Texture* specularGlossinessTexture = nullptr;
		vrt::Texture* diffuseTexture = nullptr;

		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		Material(CoreBase* coreBase) : pCoreBase(coreBase){};
		void createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags);
	};

	// -- primitive struct 
	struct Primitive {
		uint32_t firstIndex;
		uint32_t indexCount;
		uint32_t firstVertex;
		uint32_t vertexCount;
		Material& material;

		// -- dimensions struct
		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		void setDimensions(glm::vec3 min, glm::vec3 max);
		Primitive(uint32_t firstIndex, uint32_t indexCount, Material& material) : firstIndex(firstIndex), indexCount(indexCount), material(material) {};
	};

	// -- mesh struct 
	struct Mesh {
		CoreBase* pCoreBase = nullptr;

		std::vector<Primitive*> primitives;
		std::string name;

		struct UniformBuffer {
			//VkBuffer buffer;
			//VkDeviceMemory memory;
			vrt::Buffer ubo;

			VkDescriptorBufferInfo descriptor;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			//void* mapped;
		} uniformBuffer;

		struct UniformBlock {
			glm::mat4 matrix;
			glm::mat4 jointMatrix[64]{};
			float jointcount{ 0 };
		} uniformBlock;

		Mesh(CoreBase* coreBase, glm::mat4 matrix, std::string);
		//~Mesh();
	};

	// -- skin struct 
	struct Skin {
		std::string name;
		Node* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<Node*> joints;
	};

	// -- node struct 
	struct Node {
		Node* parent = nullptr;
		uint32_t index = 0;
		std::vector<Node*> children;
		glm::mat4 matrix;
		std::string name = " ";
		Mesh* mesh;
		Skin* skin;
		int32_t skinIndex = -1;
		glm::vec3 translation{};
		glm::vec3 scale{ 1.0f };
		glm::quat rotation{};
		glm::mat4 localMatrix();
		glm::mat4 getMatrix();
		bool hasMesh = false;
		void update();
		//~Node();
	};


	// -- animation channel struct  
	struct AnimationChannel {
		enum PathType { TRANSLATION, ROTATION, SCALE };
		PathType path;
		Node* node;
		uint32_t samplerIndex;
	};

	// -- animation sampler struct 
	struct AnimationSampler {
		enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
		InterpolationType interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	// -- animation struct
	struct Animation {
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
	};


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
		static VkVertexInputBindingDescription vertexInputBindingDescription;
		static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions;
		static VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo;
		static VkVertexInputBindingDescription inputBindingDescription(uint32_t binding);
		static VkVertexInputAttributeDescription inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component);
		static std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components);
		/** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
		static VkPipelineVertexInputStateCreateInfo* getPipelineVertexInputState(const std::vector<VertexComponent> components);
	};

	// -- file loading flags
	enum FileLoadingFlags {
		None = 0x00000000,
		PreTransformVertices = 0x00000001,
		PreMultiplyVertexColors = 0x00000002,
		FlipY = 0x00000004,
		DontLoadImages = 0x00000008
	};

	// -- render flags
	enum RenderFlags {
		BindImages = 0x00000001,
		RenderOpaqueNodes = 0x00000002,
		RenderAlphaMaskedNodes = 0x00000004,
		RenderAlphaBlendedNodes = 0x00000008
	};

	// -- Model class
	//@brief model loading class to handle gltf format
	class TFModel {

	private:
		vrt::Texture* getTexture(uint32_t index);
		vrt::Texture emptyTexture;
		//void createEmptyTexture(VkQueue transferQueue);

		// -- buffer list 
		//@brief for DESTROYING the buffers when they are hard to find
		std::vector<vrt::Buffer> bufferList;

	public:
		// -- mesh class
		TFMesh tfMesh;

		//vks::VulkanDevice* device;
		VkDescriptorPool descriptorPool;

		struct Vertices {
			int count;
			vrt::Buffer verticesBuffer{};
		};

		Vertices vertices{};

		struct Indices {
			int count;
			vrt::Buffer indicesBuffer{};
		};

		Indices indices{};

		std::vector<Node> nodes;
		std::vector<Node> linearNodes;

		std::vector<Skin> skins;

		std::vector<vrt::Texture> textures;
		std::vector<Material> materials;
		std::vector<Animation> animations;

		struct Dimensions {
			glm::vec3 min = glm::vec3(FLT_MAX);
			glm::vec3 max = glm::vec3(-FLT_MAX);
			glm::vec3 size;
			glm::vec3 center;
			float radius;
		} dimensions;

		bool metallicRoughnessWorkflow = true;
		bool buffersBound = false;

		std::string path;

		// -- core pointer
		CoreBase* pCoreBase = nullptr;

		// -- default constructor
		TFModel();

		// -- constructor initializes core pointer
		TFModel(CoreBase* coreBase, std::string filePath);

		// -- init
		//@brief initializes members
		void InitTFModel(CoreBase* coreBase, std::string filePath);

		void loadNode(vrt::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex,
		  const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale);
		 
		void loadSkins(tinygltf::Model& model);
		void loadImages(tinygltf::Model& model, VkQueue transferQueue);
		void loadMaterials(tinygltf::Model& model);
		void loadAnimations(tinygltf::Model& model);
		void loadFromFile(std::string filename, CoreBase* coreBase, VkQueue transferQueue, uint32_t fileLoadingFlags = vrt::FileLoadingFlags::None, float scale = 1.0f);
		//void bindBuffers(VkCommandBuffer commandBuffer);
		//void drawNode(Node* node, VkCommandBuffer commandBuffer, uint32_t renderFlags = 0, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageSet = 1);
		//void draw(VkCommandBuffer commandBuffer, uint32_t renderFlags = 0, VkPipelineLayout pipelineLayout = VK_NULL_HANDLE, uint32_t bindImageSet = 1);
		void getNodeDimensions(Node* node, glm::vec3& min, glm::vec3& max);
		void getSceneDimensions();
		//void updateAnimation(uint32_t index, float time);
		Node* findNode(Node* parent, uint32_t index);
		Node* nodeFromIndex(uint32_t index);
		void prepareNodeDescriptor(vrt::Node* node, VkDescriptorSetLayout descriptorSetLayout, int descriptorIDX);

		// -- destroy
		void DestroyTFModel();

	};

}
