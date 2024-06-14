#pragma once

#include <EngineCore.hpp>
#include <Shader.hpp>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <imconfig.h>
#include <imgui.h>
#include <imgui_internal.h>

#include <filesystem>

class Utilities_UI {

public:

  struct TransformMatrices {
    glm::mat4 rotate = glm::mat4(1.0f);
    glm::mat4 translate = glm::mat4(1.0f);
    glm::mat4 scale = glm::mat4(1.0f);
  };

  struct TransformValues {
    glm::vec4 rotate = glm::vec4(0.0f);
    glm::vec4 translate = glm::vec4(0.0f);
    glm::vec4 scale = glm::vec4(0.0f);
  };

  struct ModelData {
    bool isUpdated = false;
    int modelIndex = 0;
    std::vector<std::string> modelName;
    std::vector<TransformMatrices> transformMatrices{};
    std::vector<TransformValues> transformValues{};
  };

};
