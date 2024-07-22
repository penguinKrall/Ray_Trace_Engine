#pragma once

#include <EngineCore.hpp>
#include <Utilities_EngCore.hpp>

#include <filesystem>
#include <iostream>

#include <ktx.h>
#include <ktxvulkan.h>

#include <tiny_gltf.h>

namespace gtp {

// - texture class
//@brief
class TextureLoader {

public:
  VkImage image;
  VkImageLayout imageLayout;
  VkDeviceMemory imageMemory;
  VkImageView view;
  uint32_t width, height;
  uint32_t mipLevels;
  uint32_t layerCount;
  VkDescriptorImageInfo descriptor;
  VkSampler sampler;

  uint32_t index = -1;

  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // -- default ctor
  TextureLoader();

  // -- init ctor
  TextureLoader(EngineCore *pEngineCore);

  // -- init func
  void InitTextureLoader(EngineCore *pEngineCore);

  // -- update descriptor
  void updateDescriptor();

  ktxResult loadKTXFile(std::string filename, ktxTexture **target);

  // -- load from file
  //@brief test func to make sure loading ktx files is possible
  void loadFromFile(
      std::string filename, VkFormat format,
      VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT,
      VkImageLayout imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      bool forceLinear = false);

  void DestroyTextureLoader();

  void LoadTextureFromGLTF(tinygltf::Image &gltfimage, std::string path,
                     EngineCore *pEngineCore, VkQueue copyQueue);

// -- load texture file
// -- for loading non ktx textures
stbi_uc* LoadTextureFile(const std::string& fileName, int* width, int* height, VkDeviceSize* imageSize);

// -- load cube map
void LoadCubemap(std::string cubeMapFolderPath);
};

} // namespace gtp
