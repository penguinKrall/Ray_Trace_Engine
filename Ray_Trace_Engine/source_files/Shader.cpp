#include "Shader.hpp"

namespace gtp {

Shader::Shader() {}

Shader::Shader(EngineCore *pEngineCore) { InitShader(pEngineCore); }

void Shader::InitShader(EngineCore *pEngineCore) { this->pEngineCore = pEngineCore; }

VkShaderModule Shader::loadShaderModule(const char *fileName, VkDevice device,
                                        std::string moduleName) {

  std::ifstream file(fileName, std::ios::binary | std::ios::ate);

  if (file.is_open()) {
    auto fileSize = (size_t)file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> fileBuffer(fileSize); // shaderCode = new char[fileSize];
    file.read(fileBuffer.data(), fileSize);
    file.close();

    assert(fileSize > 0);

    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo moduleCreateInfo{};
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.codeSize = fileSize;
    moduleCreateInfo.pCode =
        reinterpret_cast<const uint32_t *>(fileBuffer.data());

    pEngineCore->AddObject(
        [this, &moduleCreateInfo, &shaderModule]() {
          return pEngineCore->objCreate.VKCreateShaderModule(
              &moduleCreateInfo, nullptr, &shaderModule);
        },
        moduleName);

    // if (vkCreateShaderModule(device, &moduleCreateInfo, NULL, &shaderModule)
    // != VK_SUCCESS) { 	throw std::invalid_argument("failed to create shader
    //module");
    // }

    // delete[] shaderCode;

    return shaderModule;
  } else {
    std::cerr << "Error: Could not open shader file \"" << fileName << "\""
              << "\n";
    return VK_NULL_HANDLE;
  }
}

VkPipelineShaderStageCreateInfo Shader::loadShader(std::string fileName,
                                                   VkShaderStageFlagBits stage,
                                                   std::string moduleName) {
  VkPipelineShaderStageCreateInfo shaderStage = {};
  shaderStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStage.stage = stage;
  shaderStage.module = loadShaderModule(
      fileName.c_str(), pEngineCore->devices.logical, moduleName);
  shaderStage.pName = "main";
  assert(shaderStage.module != VK_NULL_HANDLE);
  shaderModules.push_back(shaderStage.module);
  return shaderStage;
}

void Shader::DestroyShader() {

  for (const auto &shadmod : shaderModules) {
    vkDestroyShaderModule(pEngineCore->devices.logical, shadmod, nullptr);
  }
}
} // namespace gtp
