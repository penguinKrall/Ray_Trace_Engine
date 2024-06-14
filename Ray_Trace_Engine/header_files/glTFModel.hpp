#pragma once

#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

#include "EngineCore.hpp"
#include "vulkan/vulkan.h"

#include <ktx.h>
#include <ktxvulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "tiny_gltf.h"

#define MAX_NUM_JOINTS 128u

namespace gtp {

enum FileLoadingFlags {
  None = 0x00000000,
  PreTransformVertices = 0x00000001,
  PreMultiplyVertexColors = 0x00000002,
  FlipY = 0x00000004,
  DontLoadImages = 0x00000008
};

struct Node;

struct BoundingBox {
  glm::vec3 min;
  glm::vec3 max;
  bool valid = false;
  BoundingBox();
  BoundingBox(glm::vec3 min, glm::vec3 max);
  BoundingBox getAABB(glm::mat4 m);
};

struct TextureSampler {
  VkFilter magFilter;
  VkFilter minFilter;
  VkSamplerAddressMode addressModeU;
  VkSamplerAddressMode addressModeV;
  VkSamplerAddressMode addressModeW;
};

struct Texture {
  EngineCore *coreBase;
  VkImage image{};
  VkImageLayout imageLayout;
  VkDeviceMemory deviceMemory;
  VkImageView view;
  uint32_t width, height;
  uint32_t mipLevels;
  uint32_t layerCount;
  VkDescriptorImageInfo descriptor;
  VkSampler sampler;
  uint32_t index;
  void updateDescriptor();
  void destroy();
  // Load a texture from a glTF image (stored as vector of chars loaded via
  // stb_image) and generate a full mip chaing for it
  void fromglTfImage(tinygltf::Image &gltfimage, TextureSampler textureSampler,
                     EngineCore *coreBase, VkQueue copyQueue);
};

struct Material {
  enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
  AlphaMode alphaMode = ALPHAMODE_OPAQUE;
  float alphaCutoff = 1.0f;
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;
  glm::vec4 baseColorFactor = glm::vec4(1.0f);
  glm::vec4 emissiveFactor = glm::vec4(0.0f);
  gtp::Texture *baseColorTexture;
  gtp::Texture *metallicRoughnessTexture;
  gtp::Texture *normalTexture;
  gtp::Texture *occlusionTexture;
  gtp::Texture *emissiveTexture;
  bool doubleSided = false;
  struct TexCoordSets {
    uint8_t baseColor = 0;
    uint8_t metallicRoughness = 0;
    uint8_t specularGlossiness = 0;
    uint8_t normal = 0;
    uint8_t occlusion = 0;
    uint8_t emissive = 0;
  } texCoordSets;
  struct Extension {
    gtp::Texture *specularGlossinessTexture;
    gtp::Texture *diffuseTexture;
    glm::vec4 diffuseFactor = glm::vec4(1.0f);
    glm::vec3 specularFactor = glm::vec3(0.0f);
  } extension;
  struct PbrWorkflows {
    bool metallicRoughness = true;
    bool specularGlossiness = false;
  } pbrWorkflows;
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  int index = 0;
  bool unlit = false;
  float emissiveStrength = 1.0f;
};

struct Primitive {
  uint32_t firstIndex;
  uint32_t indexCount;
  uint32_t firstVertex;
  uint32_t vertexCount;
  Material &material;
  bool hasIndices;
  BoundingBox bb;
  Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount,
            Material &material);
  void setBoundingBox(glm::vec3 min, glm::vec3 max);
};

struct Mesh {
  EngineCore *coreBase;
  std::vector<Primitive *> primitives;
  BoundingBox bb;
  BoundingBox aabb;
  struct UniformBuffer {
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDescriptorBufferInfo descriptor;
    VkDescriptorSet descriptorSet;
    void *mapped;
  } uniformBuffer;
  struct UniformBlock {
    glm::mat4 matrix;
    glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
    float jointcount{0};
  } uniformBlock;
  Mesh(EngineCore *coreBase, glm::mat4 matrix);
  ~Mesh();
  void setBoundingBox(glm::vec3 min, glm::vec3 max);
};

struct Skin {
  std::string name;
  Node *skeletonRoot = nullptr;
  std::vector<glm::mat4> inverseBindMatrices;
  std::vector<Node *> joints;
};

struct Node {
  Node *parent;
  uint32_t index;
  std::vector<Node *> children;
  glm::mat4 matrix;
  std::string name;
  Mesh *mesh;
  Skin *skin;
  int32_t skinIndex = -1;
  glm::vec3 translation{};
  glm::vec3 scale{1.0f};
  glm::quat rotation{};
  BoundingBox bvh;
  BoundingBox aabb;
  glm::mat4 localMatrix();
  glm::mat4 getMatrix();
  void update();
  ~Node();
};

struct AnimationChannel {
  enum PathType { TRANSLATION, ROTATION, SCALE };
  PathType path;
  Node *node;
  uint32_t samplerIndex;
};

struct AnimationSampler {
  enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
  InterpolationType interpolation;
  std::vector<float> inputs;
  std::vector<glm::vec4> outputsVec4;
};

struct Animation {
  std::string name;
  std::vector<AnimationSampler> samplers;
  std::vector<AnimationChannel> channels;
  float start = std::numeric_limits<float>::max();
  float end = std::numeric_limits<float>::min();
  float currentTime = 0.0f;
};

struct Model {

  EngineCore *coreBase;

  std::string modelName;

  struct Vertex {
    glm::vec4 pos;
    glm::vec4 normal;
    glm::vec2 uv0;
    glm::vec2 uv1;
    glm::vec4 color;
    glm::vec4 joint0;
    glm::vec4 weight0;
    glm::vec4 tangent;
  };

  struct Vertices {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory;
  } vertices;

  struct Indices {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory;
  } indices;

  glm::mat4 aabb;

  std::vector<Node *> nodes;
  std::vector<Node *> linearNodes;

  std::vector<Skin *> skins;

  std::vector<Texture> textures;
  std::vector<TextureSampler> textureSamplers;
  std::vector<Material> materials;

  std::vector<Animation> animations;

  std::vector<std::string> extensions;

  struct Dimensions {
    glm::vec3 min = glm::vec3(FLT_MAX);
    glm::vec3 max = glm::vec3(-FLT_MAX);
  } dimensions;

  struct LoaderInfo {
    uint32_t *indexBuffer;
    Vertex *vertexBuffer;
    size_t indexPos = 0;
    size_t vertexPos = 0;
  };

  LoaderInfo loaderInfo{};
  size_t vertexCount = 0;
  size_t indexCount = 0;
  size_t vertexBufferSize = 0;
  size_t indexBufferSize = 0;
  int semiTransparentFlag = 0;

  void destroy(VkDevice device);
  void loadNode(gtp::Node *parent, const tinygltf::Node &node,
                uint32_t nodeIndex, const tinygltf::Model &model,
                LoaderInfo &loaderInfo, float globalscale);
  void getNodeProps(const tinygltf::Node &node, const tinygltf::Model &model,
                    size_t &vertexCount, size_t &indexCount);
  void loadSkins(tinygltf::Model &gltfModel);
  void loadTextures(tinygltf::Model &gltfModel, EngineCore *coreBase,
                    VkQueue transferQueue);
  VkSamplerAddressMode getVkWrapMode(int32_t wrapMode);
  VkFilter getVkFilterMode(int32_t filterMode);
  void loadTextureSamplers(tinygltf::Model &gltfModel);
  void loadMaterials(tinygltf::Model &gltfModel);
  void loadAnimations(tinygltf::Model &gltfModel);
  void loadFromFile(std::string filename, EngineCore *coreBase,
                    VkQueue transferQueue,
                    uint32_t fileLoadingFlags = FileLoadingFlags::None,
                    float scale = 1.0f);
  void drawNode(Node *node, VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);
  void calculateBoundingBox(Node *node, Node *parent);
  void getSceneDimensions();
  void updateAnimation(uint32_t index, float time);
  Node *findNode(Node *parent, uint32_t index);
  Node *nodeFromIndex(uint32_t index);
};
} // namespace gtp
