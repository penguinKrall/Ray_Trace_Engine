#pragma once

#include <EngineCore.hpp>
#include <fstream>

namespace gtp {
// -- shader class
class Shader {
public:
  // -- core pointer
  //@brief seems like its going to be helpful. delete later if not.
  EngineCore *pEngineCore = nullptr;

  // -- list of shader modules created (stored for cleanup)
  std::vector<VkShaderModule> shaderModules;

  // -- constructor
  Shader();
  Shader(EngineCore *pEngineCore);

  // -- init function
  void InitShader(EngineCore *pEngineCore);

  // -- load shader
  //@return VkShaderModule
  VkShaderModule loadShaderModule(const char *fileName, VkDevice device,
                                  std::string moduleName);

  VkPipelineShaderStageCreateInfo loadShader(std::string fileName,
                                             VkShaderStageFlagBits stage,
                                             std::string moduleName);

  void DestroyShader();
};

} // namespace gtp
