#pragma once

#include <CoreBase.hpp>
#include <fstream>

// -- shader class
class Shader {
public:

	// -- core pointer
	//@brief seems like its going to be helpful. delete later if not. 
	CoreBase* pCoreBase = nullptr;

	// -- list of shader modules created (stored for cleanup)
	std::vector<VkShaderModule> shaderModules;

	// -- constructor
	Shader();
	Shader(CoreBase* coreBase);

	// -- init function
	void InitShader(CoreBase* coreBase);

	// -- load shader
	//@return VkShaderModule
	VkShaderModule loadShaderModule(const char* fileName, VkDevice device, std::string moduleName);

	VkPipelineShaderStageCreateInfo loadShader(std::string fileName, VkShaderStageFlagBits stage, std::string moduleName);

	void DestroyShader();
};

