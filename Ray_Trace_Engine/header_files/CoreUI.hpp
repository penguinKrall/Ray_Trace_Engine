#pragma once

#include <Utilities_UI.hpp>

// -- class to contain ui data -- dear_imgui
class CoreUI {

public:
  // -- core pointer
  EngineCore *pEngineCore = nullptr;

  // shader
  gtp::Shader shader;

  // -- image data struct
  struct UIImageData {
    int texWidth = 0;
    int texHeight = 0;
    VkImage image{};
    VkImageView view{};
    VkDeviceMemory memory{};
    VkSampler sampler{};
  };

  // -- push constant struct
  struct UIPushConstant {
    glm::vec2 scale;
    glm::vec2 translate;
  };

  // -- buffers struct
  struct UIBuffers {
    gtp::Buffer vertex{};
    gtp::Buffer index{};
    int32_t vertexCount = 0;
    int32_t indexCount = 0;
  };

  // -- descriptor struct
  struct UIDescriptor {
    VkDescriptorPool descriptorPool{};
    VkDescriptorSetLayout descriptorSetLayout{};
    VkDescriptorSet descriptorSet{};
  };

  // -- render data struct
  struct UIRenderData {
    VkRenderPass renderPass{};
    std::vector<VkFramebuffer> framebuffer{};
    VkPipeline pipeline{};
    VkPipelineLayout pipelineLayout{};
    VkPipelineCache pipelineCache{};
  };

  // -- style struct
  struct UIStyle {
    float alpha = 1.0f;
    float bgOpacity = 0.5f;
  };

  struct UIProperties {
    bool visible = true;
    bool updated = false;
    float scale = 1.0f;
    uint32_t subpass = 0;
  };

  // -- imgui backends struct
  struct UIBackends {
    ImGuiContext *context = nullptr;
    ImGuiIO *io = nullptr;
    ImDrawData *drawData = nullptr;
  };

  // font data
  unsigned char *fontData = nullptr;

  // class member structs
  UIImageData fontImage{};
  UIPushConstant pushConstant{};
  UIBuffers buffers{};
  UIDescriptor descriptor{};
  UIRenderData renderData{};
  UIStyle style{};
  UIProperties properties{};
  UIBackends backends{};

  quat qRot = quat(1.f, 0.f, 0.f, 0.f);

  //--utilities structs
  //model data
  Utilities_UI::ModelData modelData{};
  
  // -- renderer data
  Utilities_UI::RenderData rendererData{};

  // -- ctors
  CoreUI();
  CoreUI(EngineCore *coreBase);

  // -- initializers
  // -- init context
  //@brief sets context for imgui - glfwwindow
  void InitContext();

  // -- init style
  //@brief sets style/colors/scale for ui window
  void InitStyle();

  // -- init core ui
  //@brief sets core pointer and calls initializer funcs
  void InitCoreUI(EngineCore *coreBase);

  // -- create funcs
  // -- create font image
  //@brief creates font image
  void CreateFontImage();

  // -- create font sampler
  //@brief creates font sampler
  void CreateFontSampler();

  // -- create render pass
  //@brief creates UI render pass
  void CreateUIRenderPass();

  // -- create pipeline/layout
  //@brief creates UI pipeline and pipeline layout
  void CreateUIPipeline();

  // -- create framebuffer
  void CreateUIFramebuffer();

  // -- recreate framebuffer
  void RecreateFramebuffers();

  // -- create resources
  //@brief calls create funcs
  void CreateResources();

  // -- create font image descriptor
  //@brief creates descriptor pool, set layout, and set for UI font
  void CreateFontImageDescriptor();

  // -- update
  //@brief update vertex and index buffer containing the imGui elements when
  // required
  void UpdateBuffers();

  // -- input
  //@brief creates input window draw data
  void Input(Utilities_UI::ModelData* pModelData);

  // -- draw ui
  //@brief binds render data and draws vertex/index buffers
  void DrawUI(const VkCommandBuffer commandBuffer, int currentFrame);

  // -- set model data
  void SetModelData(const Utilities_UI::ModelData* pModelData);

  // -- destroy ui
  void DestroyUI();
};
