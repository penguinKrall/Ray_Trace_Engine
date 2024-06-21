#include "CoreUI.hpp"

CoreUI::CoreUI() {}

CoreUI::CoreUI(EngineCore *coreBase) {
  shader = gtp::Shader(coreBase);
  //this->buffers.vertex.resize(frame_draws);
  //this->buffers.index.resize(frame_draws);
  InitContext();
  InitStyle();
  InitCoreUI(coreBase);
  CreateResources();

  // check version
  IMGUI_CHECKVERSION();

  //// 2: initialize imgui library
  backends.io = &ImGui::GetIO();
  backends.io->DisplaySize.x = static_cast<float>(this->pEngineCore->width);
  backends.io->DisplaySize.y = static_cast<float>(this->pEngineCore->height);

  // initialize ImGui for GLFW
  ImGui_ImplGlfw_InitForVulkan(pEngineCore->windowGLFW, true);

  // init for vulkan
  ImGui_ImplVulkan_InitInfo imguiInitInfo = {};
  imguiInitInfo.Instance = pEngineCore->instance;
  imguiInitInfo.PhysicalDevice = pEngineCore->devices.physical;
  imguiInitInfo.Device = pEngineCore->devices.logical;
  imguiInitInfo.Queue = pEngineCore->queue.graphics;
  imguiInitInfo.DescriptorPool = descriptor.descriptorPool;
  imguiInitInfo.MinImageCount = 3;
  imguiInitInfo.ImageCount = 3;
  imguiInitInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  imguiInitInfo.Subpass = 0;

  // init vulkan backends
  ImGui_ImplVulkan_Init(&imguiInitInfo, renderData.renderPass);
}

void CoreUI::InitContext() { backends.context = ImGui::CreateContext(); }

void CoreUI::InitStyle() {
  ImGuiStyle &imguiStyle = ImGui::GetStyle();

  imguiStyle.Alpha = style.alpha;
  imguiStyle.FrameRounding = 3.0f;
  imguiStyle.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
  imguiStyle.Colors[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
  imguiStyle.Colors[ImGuiCol_WindowBg] =
      ImVec4(0.0f, 0.0f, 0.0f, style.bgOpacity);
  imguiStyle.Colors[ImGuiCol_PopupBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
  imguiStyle.Colors[ImGuiCol_Border] = ImVec4(0.00f, 0.00f, 0.00f, 0.0f);
  imguiStyle.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 1.0f);
  imguiStyle.Colors[ImGuiCol_FrameBg] = ImVec4(1.00f, 1.00f, 1.00f, 0.25f);
  imguiStyle.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.5f, 0.5f, 0.5f, 0.50f);
  imguiStyle.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
  imguiStyle.Colors[ImGuiCol_TitleBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
  imguiStyle.Colors[ImGuiCol_TitleBgCollapsed] =
      ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
  imguiStyle.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
  imguiStyle.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.86f, 0.86f, 0.86f, 0.0f);
  imguiStyle.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
  imguiStyle.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
  imguiStyle.Colors[ImGuiCol_ScrollbarGrabHovered] =
      ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
  imguiStyle.Colors[ImGuiCol_ScrollbarGrabActive] =
      ImVec4(1.0f, 1.0f, 1.0f, 0.5f);
  imguiStyle.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
  imguiStyle.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
  imguiStyle.Colors[ImGuiCol_SliderGrabActive] =
      ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
  imguiStyle.Colors[ImGuiCol_Button] = ImVec4(1.00f, 1.00f, 1.00f, 0.40f);
  imguiStyle.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
  imguiStyle.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
  imguiStyle.Colors[ImGuiCol_Header] = ImVec4(1.00f, 1.00f, 1.00f, 0.5f);
  imguiStyle.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
  imguiStyle.Colors[ImGuiCol_HeaderActive] = ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
  imguiStyle.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.00f, 1.00f, 1.00f, 0.50f);
  imguiStyle.Colors[ImGuiCol_ResizeGripHovered] =
      ImVec4(0.5f, 0.5f, 0.5f, 0.5f);
  imguiStyle.Colors[ImGuiCol_ResizeGripActive] =
      ImVec4(1.0f, 1.0f, 1.0f, 1.00f);
  imguiStyle.Colors[ImGuiCol_PlotLines] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  imguiStyle.Colors[ImGuiCol_PlotLinesHovered] =
      ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
  imguiStyle.Colors[ImGuiCol_PlotHistogram] =
      ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
  imguiStyle.Colors[ImGuiCol_PlotHistogramHovered] =
      ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
  imguiStyle.Colors[ImGuiCol_TextSelectedBg] =
      ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

  for (int i = 0; i <= ImGuiCol_COUNT; i++) {
    ImVec4 &col = imguiStyle.Colors[i];
    if (col.w < 1.00f) {
      col.x *= style.alpha;
      col.y *= style.alpha;
      col.z *= style.alpha;
      col.w *= style.alpha;
    }
  }

  // Dimensions
  backends.io = &ImGui::GetIO();
  backends.io->FontGlobalScale = properties.scale;
}

void CoreUI::InitCoreUI(EngineCore *coreBase) {
  this->pEngineCore = coreBase;
  std::cout << "\ninitializing UI\n'''''''''''''''\n" << std::endl;
}

void CoreUI::CreateFontImage() {

  // font texture data
  // unsigned char* fontData = nullptr;

  // font file path
  std::filesystem::path projDirectory = std::filesystem::current_path();
  // std::cout << "*********************************Project Directory: " <<
  // projDirectory.string() << std::endl;

  // output to test
  const std::string fontFilename =
      projDirectory.string() + "/fonts/FiraCode.ttf";

  // add font file
  backends.io->Fonts->AddFontFromFileTTF(fontFilename.c_str(),
                                         16.0f * properties.scale);

  // get texture data as rgba32
  backends.io->Fonts->GetTexDataAsRGBA32(&fontData, &fontImage.texWidth,
                                         &fontImage.texHeight);

  ////SRS - Set ImGui style scale factor to handle retina and other HiDPI
  /// displays (same as font scaling above)
  ImGuiStyle &locStyle = ImGui::GetStyle();
  locStyle.ScaleAllSizes(properties.scale);

  ////create font image/memory/view
  // CreateFontImage();

  // create target image for copy
  VkImageCreateInfo targetImageCreateInfo{};
  targetImageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  targetImageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  targetImageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  targetImageCreateInfo.extent.width =
      static_cast<uint32_t>(fontImage.texWidth);
  targetImageCreateInfo.extent.height =
      static_cast<uint32_t>(fontImage.texHeight);
  targetImageCreateInfo.extent.depth = 1;
  targetImageCreateInfo.mipLevels = 1;
  targetImageCreateInfo.arrayLayers = 1;
  targetImageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  targetImageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  targetImageCreateInfo.usage =
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  targetImageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  targetImageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  // create image
  pEngineCore->add(
      [this, &targetImageCreateInfo]() {
        return pEngineCore->objCreate.VKCreateImage(&targetImageCreateInfo,
                                                    nullptr, &fontImage.image);
      },
      "UIFontImage");

  // image memory requirements
  VkMemoryRequirements memReqs;
  vkGetImageMemoryRequirements(pEngineCore->devices.logical, fontImage.image,
                               &memReqs);

  // image memory allocation information
  VkMemoryAllocateInfo memoryAllocateInfo{};
  memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  memoryAllocateInfo.allocationSize = memReqs.size;
  memoryAllocateInfo.memoryTypeIndex = pEngineCore->getMemoryType(
      memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  // allocate image memory
  pEngineCore->add(
      [this, &memoryAllocateInfo]() {
        return pEngineCore->objCreate.VKAllocateMemory(
            &memoryAllocateInfo, nullptr, &fontImage.memory);
      },
      "UIFontImageMemory");

  // bind image memory
  if (vkBindImageMemory(pEngineCore->devices.logical, fontImage.image,
                        fontImage.memory, 0) != VK_SUCCESS) {
    throw std::invalid_argument("failed to bind image memory");
  }

  // create image view
  VkImageViewCreateInfo fontImageViewCreateInfo{};
  fontImageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  fontImageViewCreateInfo.image = fontImage.image;
  fontImageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  fontImageViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  fontImageViewCreateInfo.subresourceRange.aspectMask =
      VK_IMAGE_ASPECT_COLOR_BIT;
  fontImageViewCreateInfo.subresourceRange.levelCount = 1;
  fontImageViewCreateInfo.subresourceRange.layerCount = 1;

  // create image view and map name/handle for debug
  pEngineCore->add(
      [this, &fontImageViewCreateInfo]() {
        return pEngineCore->objCreate.VKCreateImageView(
            &fontImageViewCreateInfo, nullptr, &fontImage.view);
      },
      "UIFontImageView");
}

void CoreUI::CreateFontSampler() {
  // Font texture Sampler
  VkSamplerCreateInfo samplerInfo{};
  samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter = VK_FILTER_LINEAR;
  samplerInfo.minFilter = VK_FILTER_LINEAR;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias = 0.0f;
  samplerInfo.minLod = 0.0f;
  samplerInfo.maxLod = 1.0f;
  samplerInfo.anisotropyEnable = VK_TRUE;
  samplerInfo.maxAnisotropy = 8;

  pEngineCore->add(
      [this, &samplerInfo]() {
        return pEngineCore->objCreate.VKCreateSampler(&samplerInfo, nullptr,
                                                      &fontImage.sampler);
      },
      "fontImageSampler");
}

void CoreUI::CreateUIRenderPass() {

  // attachments (swapchain image)
  std::array<VkAttachmentDescription, 1> attachmentDescription{};
  attachmentDescription[0].format =
      pEngineCore->swapchainData.assignedSwapchainImageFormat.format;
  attachmentDescription[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescription[0].loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
  attachmentDescription[0].storeOp = VK_ATTACHMENT_STORE_OP_NONE;
  attachmentDescription[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescription[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescription[0].initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  attachmentDescription[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  // attachment reference
  std::vector<VkAttachmentReference> swapchainImageReference = {
      {0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}};

  // subpass description
  VkSubpassDescription subpassDescription{};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount =
      static_cast<uint32_t>(swapchainImageReference.size());
  subpassDescription.pColorAttachments = swapchainImageReference.data();
  subpassDescription.inputAttachmentCount = 0;

  // subpass dependencies
  std::array<VkSubpassDependency, 2> subpassDependencies;
  subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[0].srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[0].dstSubpass = 0;
  subpassDependencies[0].dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[0].dependencyFlags = 0;

  subpassDependencies[1].srcSubpass = 0;
  subpassDependencies[1].srcStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  subpassDependencies[1].dstStageMask =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  subpassDependencies[1].dependencyFlags = 0;

  // create info
  VkRenderPassCreateInfo renderPassCreateInfo{};
  renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassCreateInfo.attachmentCount =
      static_cast<uint32_t>(attachmentDescription.size());
  renderPassCreateInfo.pAttachments = attachmentDescription.data();
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpassDescription;
  renderPassCreateInfo.dependencyCount =
      static_cast<uint32_t>(subpassDependencies.size());
  renderPassCreateInfo.pDependencies = subpassDependencies.data();

  // create render pass
  pEngineCore->add(
      [this, &renderPassCreateInfo]() {
        return pEngineCore->objCreate.VKCreateRenderPass(
            &renderPassCreateInfo, nullptr, &renderData.renderPass);
      },
      "UIRenderPass");
}

void CoreUI::CreateUIPipeline() {

  // push constant range
  VkPushConstantRange pushConstantRange{};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(UIPushConstant);

  // pipeline layout create info
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
  pipelineLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &descriptor.descriptorSetLayout;
  pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
  pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

  // create pipeline layout
  pEngineCore->add(
      [this, &pipelineLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreatePipelineLayout(
            &pipelineLayoutCreateInfo, nullptr, &renderData.pipelineLayout);
      },
      "UIPipelineLayout");

  // input assembly state
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
  inputAssemblyStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;
  inputAssemblyStateCreateInfo.flags = 0;

  // rasterization state
  VkPipelineRasterizationStateCreateInfo rasterizationState{};
  rasterizationState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizationState.depthClampEnable = VK_FALSE;
  rasterizationState.rasterizerDiscardEnable = VK_FALSE;
  rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
  rasterizationState.lineWidth = 1.0f;
  rasterizationState.cullMode = VK_CULL_MODE_NONE;
  rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizationState.depthBiasEnable = VK_FALSE;

  // blend attachment state
  VkPipelineColorBlendAttachmentState blendAttachmentStateCreateInfo{};
  blendAttachmentStateCreateInfo.blendEnable = VK_TRUE;
  blendAttachmentStateCreateInfo.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  blendAttachmentStateCreateInfo.srcColorBlendFactor =
      VK_BLEND_FACTOR_SRC_ALPHA;
  blendAttachmentStateCreateInfo.dstColorBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachmentStateCreateInfo.colorBlendOp = VK_BLEND_OP_ADD;
  blendAttachmentStateCreateInfo.srcAlphaBlendFactor =
      VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  blendAttachmentStateCreateInfo.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  blendAttachmentStateCreateInfo.alphaBlendOp = VK_BLEND_OP_ADD;

  VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
  colorBlendStateCreateInfo = {};
  colorBlendStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
  colorBlendStateCreateInfo.attachmentCount = 1;
  colorBlendStateCreateInfo.pAttachments = &blendAttachmentStateCreateInfo;

  VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
  depthStencilStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
  depthStencilStateCreateInfo.depthWriteEnable = VK_TRUE;
  depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_ALWAYS;
  depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
  depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

  VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
  viewportStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportStateCreateInfo.viewportCount = 1;
  viewportStateCreateInfo.pViewports = &pEngineCore->viewport;
  viewportStateCreateInfo.scissorCount = 1;
  viewportStateCreateInfo.pScissors = &pEngineCore->scissor;
  viewportStateCreateInfo.flags = 0;

  VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
  multisampleStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampleStateCreateInfo.sampleShadingEnable = false;
  multisampleStateCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  multisampleStateCreateInfo.minSampleShading = 0.2f;

  std::vector<VkDynamicState> dynamicStateEnables = {VK_DYNAMIC_STATE_VIEWPORT,
                                                     VK_DYNAMIC_STATE_SCISSOR};

  VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
  dynamicStateCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  dynamicStateCreateInfo.dynamicStateCount =
      static_cast<uint32_t>(dynamicStateEnables.size());
  dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();

  VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
  pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

  // shader stages
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

  // project directory for loading shader modules
  std::filesystem::path projDirectory = std::filesystem::current_path();

  // std::cout << "Project Directory: " << projDirectory.string() +
  // "/shaders/compiled/UIOverlay.vert.spv" << std::endl;

  // vertex
  shaderStages.push_back(shader.loadShader(
      projDirectory.string() + "/shaders/compiled/ui_vertex.vert.spv",
      VK_SHADER_STAGE_VERTEX_BIT, "UIVertexShaderModule"));

  // fragment
  shaderStages.push_back(shader.loadShader(
      projDirectory.string() + "/shaders/compiled/ui_fragment.frag.spv",
      VK_SHADER_STAGE_FRAGMENT_BIT, "UIFragmentShaderModule"));

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyStateCreateInfo;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
  pipelineCreateInfo.pMultisampleState = &multisampleStateCreateInfo;
  pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
  pipelineCreateInfo.pDepthStencilState = &depthStencilStateCreateInfo;
  pipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
  pipelineCreateInfo.renderPass = renderData.renderPass;
  pipelineCreateInfo.layout = renderData.pipelineLayout;
  pipelineCreateInfo.stageCount = 2;
  pipelineCreateInfo.pStages = shaderStages.data();
  pipelineCreateInfo.subpass = properties.subpass;

#if defined(VK_KHR_dynamic_rendering)
  VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {};
  std::vector<VkFormat> colorFormat = {
      pEngineCore->swapchainData.assignedSwapchainImageFormat.format};

  VkFormat depthFormat = {pEngineCore->chooseSupportedFormat(
      {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)};

  if (renderData.renderPass == VK_NULL_HANDLE) {
    pipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    pipelineRenderingCreateInfo.colorAttachmentCount = 1;
    pipelineRenderingCreateInfo.pColorAttachmentFormats = colorFormat.data();
    pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
    pipelineRenderingCreateInfo.stencilAttachmentFormat = depthFormat;
    pipelineCreateInfo.pNext = &pipelineRenderingCreateInfo;
  }

#endif

  // Vertex bindings an attributes based on ImGui vertex definition
  std::vector<VkVertexInputBindingDescription> vertexInputBindings = {
      {0, sizeof(ImDrawVert), VK_VERTEX_INPUT_RATE_VERTEX}};

  std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = {
      {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, pos)},
      {1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(ImDrawVert, uv)},
      {2, 0, VK_FORMAT_R8G8B8A8_UNORM, offsetof(ImDrawVert, col)}};

  VkPipelineVertexInputStateCreateInfo vertexInputState{};
  vertexInputState.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputState.vertexBindingDescriptionCount =
      static_cast<uint32_t>(vertexInputBindings.size());
  vertexInputState.pVertexBindingDescriptions = vertexInputBindings.data();
  vertexInputState.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(vertexInputAttributes.size());
  vertexInputState.pVertexAttributeDescriptions = vertexInputAttributes.data();

  pipelineCreateInfo.pVertexInputState = &vertexInputState;

  pEngineCore->add(
      [this, &pipelineCreateInfo]() {
        return pEngineCore->objCreate.VKCreateGraphicsPipeline(
            renderData.pipelineCache, 1, &pipelineCreateInfo, nullptr,
            &renderData.pipeline);
      },
      "UIPipeline");
}

void CoreUI::CreateUIFramebuffer() {
  // framebuffers
  int width = 0;
  int height = 0;

  // std::cout << "msaa levels: " << rdz.core.components.msaaSamples <<
  // std::endl;

  glfwGetWindowSize(pEngineCore->windowGLFW, &width, &height);

  renderData.framebuffer.resize(frame_draws);

  int idx = 0;

  for (auto &imageviews :
       pEngineCore->swapchainData.swapchainImages.imageView) {

    std::vector<VkImageView> framebufferAttachments = {
        imageviews,
    };

    VkFramebufferCreateInfo framebufferCreateInfo = {};
    framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferCreateInfo.renderPass = renderData.renderPass;
    framebufferCreateInfo.attachmentCount =
        static_cast<uint32_t>(framebufferAttachments.size());
    framebufferCreateInfo.pAttachments = framebufferAttachments.data();
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    // Create a modified name by appending the counter value
    std::string modifiedName = std::format("{}{}", "UIFramebuffer", idx);

    pEngineCore->add(
        [this, &framebufferCreateInfo, &idx]() {
          return pEngineCore->objCreate.VKCreateFramebuffer(
              &framebufferCreateInfo, nullptr, &renderData.framebuffer[idx]);
        },
        modifiedName);

    ++idx;
  }
}

void CoreUI::RecreateFramebuffers() {
  // destroy
  for (const auto &fbuffs : renderData.framebuffer) {
    vkDestroyFramebuffer(pEngineCore->devices.logical, fbuffs, nullptr);
  }
  // create
  CreateUIFramebuffer();

  UpdateBuffers();

}

void CoreUI::CreateResources() {

  // create font image/memory/view
  CreateFontImage();

  // upload size
  VkDeviceSize uploadSize =
      fontImage.texWidth * fontImage.texHeight * 4 * sizeof(char);

  // staging buffer for font data upload
  gtp::Buffer stagingBuffer;

  // create staging buffer
  pEngineCore->add(
      [this, &stagingBuffer, &uploadSize]() {
        return stagingBuffer.CreateBuffer(
            pEngineCore->devices.physical, pEngineCore->devices.logical,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT, uploadSize,
            "UIcreateResourcesStagingBuffer");
      },
      "UIcreateResourcesStagingBuffer");

  // staging buffer memory property flags
  stagingBuffer.bufferData.memoryPropertyFlags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

  // create staging buffer memory
  pEngineCore->add(
      [this, &stagingBuffer]() {
        return stagingBuffer.AllocateBufferMemory(
            pEngineCore->devices.physical, pEngineCore->devices.logical,
            "UIcreateResourcesStagingBufferMemory");
      },
      "UIcreateResourcesStagingBufferMemory");

  // map memory
  stagingBuffer.map(pEngineCore->devices.logical, uploadSize, 0);

  // copy font data to buffer
  stagingBuffer.copyTo(fontData, uploadSize);

  // unmap memory
  stagingBuffer.unmap(pEngineCore->devices.logical);

  // bind buffer memory to buffer
  stagingBuffer.bind(pEngineCore->devices.logical, 0);

  // subresource range for font image layout transitions/copy
  VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1,
                                              0, 1};

  // create temporary command buffer for transition/copy
  VkCommandBuffer cmdBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // transition image from undefined to dst optimal
  gtp::Utilities_EngCore::setImageLayout(
      cmdBuffer, fontImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

  // buffer image copy data
  VkBufferImageCopy bufferImageCopyData = {};
  bufferImageCopyData.bufferOffset = 0;
  bufferImageCopyData.bufferRowLength = 0;
  bufferImageCopyData.bufferImageHeight = 0;
  bufferImageCopyData.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  bufferImageCopyData.imageSubresource.mipLevel = 0;
  bufferImageCopyData.imageSubresource.baseArrayLayer = 0;
  bufferImageCopyData.imageSubresource.layerCount = 1;
  bufferImageCopyData.imageOffset = {
      0,
      0,
      0,
  };
  bufferImageCopyData.imageExtent = {static_cast<uint32_t>(fontImage.texWidth),
                                     static_cast<uint32_t>(fontImage.texHeight),
                                     1};

  // copy buffer to image
  vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.bufferData.buffer,
                         fontImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                         1, &bufferImageCopyData);

  // transition image layout from dst optimal to shader read
  gtp::Utilities_EngCore::setImageLayout(
      cmdBuffer, fontImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

  // submit temporary command buffer
  pEngineCore->FlushCommandBuffer(cmdBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // destroy staging buffer
  stagingBuffer.destroy(pEngineCore->devices.logical);

  // Font texture Sampler
  CreateFontSampler();

  // font image descriptor
  CreateFontImageDescriptor();

  // render pass
  CreateUIRenderPass();

  // pipeline/layout
  CreateUIPipeline();

  // framebuffers
  CreateUIFramebuffer();
}

void CoreUI::CreateFontImageDescriptor() {

  // pool sizes
  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}};

  // create info
  VkDescriptorPoolCreateInfo descriptorPoolInfo{};
  descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolInfo.maxSets = 2;
  descriptorPoolInfo.pPoolSizes = poolSizes.data();
  descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());

  // create descriptor pool
  pEngineCore->add(
      [this, &descriptorPoolInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorPool(
            &descriptorPoolInfo, nullptr, &descriptor.descriptorPool);
      },
      "UIDescriptorPool");

  // descriptor set layout
  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
      {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
       VK_SHADER_STAGE_FRAGMENT_BIT, nullptr}};

  // descriptor set layout create info
  VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo = {
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, nullptr, 0,
      static_cast<uint32_t>(setLayoutBindings.size()),
      setLayoutBindings.data()};

  // create descriptor set layout
  pEngineCore->add(
      [this, &descriptorLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorSetLayout(
            &descriptorLayoutCreateInfo, nullptr,
            &descriptor.descriptorSetLayout);
      },
      "UIDescriptorSetLayout");

  // allocate descriptor set
  // layouts array
  std::vector<VkDescriptorSetLayout> layouts(1, descriptor.descriptorSetLayout);

  // allocate info
  VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {};
  descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocInfo.descriptorPool = descriptor.descriptorPool;
  descriptorSetAllocInfo.descriptorSetCount = 1;
  descriptorSetAllocInfo.pSetLayouts = layouts.data();

  // allocate
  if (vkAllocateDescriptorSets(pEngineCore->devices.logical,
                               &descriptorSetAllocInfo,
                               &descriptor.descriptorSet) != VK_SUCCESS) {
    throw std::invalid_argument("Failed to allocate a UI descriptor!");
  }

  // image info - descriptor
  VkDescriptorImageInfo descriptorImageInfo = {};
  descriptorImageInfo.imageView = fontImage.view;
  descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  descriptorImageInfo.sampler = fontImage.sampler;

  // write set info
  VkWriteDescriptorSet writeDescriptorSet = {};
  writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorSet.dstSet = descriptor.descriptorSet;
  writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorSet.descriptorCount = 1;
  writeDescriptorSet.dstBinding = 0;
  writeDescriptorSet.pImageInfo = &descriptorImageInfo;

  // update descriptor sets
  vkUpdateDescriptorSets(pEngineCore->devices.logical, 1, &writeDescriptorSet,
                         0, nullptr);
}

void CoreUI::UpdateBuffers() {

  // get draw data
  backends.drawData = ImGui::GetDrawData();

  // return if data is NULL
  if (!backends.drawData) {
    return;
  }

  // vertex buffer size
  VkDeviceSize vertexBufferSize =
      (static_cast<long long>(backends.drawData->TotalVtxCount * 10)) *
      (sizeof(ImDrawVert) * 2);

  // index buffer size
  VkDeviceSize indexBufferSize =
      (static_cast<long long>(backends.drawData->TotalIdxCount * 10)) *
      (sizeof(ImDrawIdx));

  // return if buffers aren't created - no need to destroy
  if ((vertexBufferSize == 0) || (indexBufferSize == 0)) {
    return;
  }

  // update vertex buffer
  if ((buffers.vertex.bufferData.buffer == VK_NULL_HANDLE) ||
      (buffers.vertexCount != backends.drawData->TotalVtxCount)) {

    // unmap
    if (buffers.vertex.bufferData.mapped != nullptr) {
      vkUnmapMemory(pEngineCore->devices.logical,
                    buffers.vertex.bufferData.memory);
      buffers.vertex.bufferData.mapped = nullptr;
    }

    // destroy buffer and allocated memory
    vkDestroyBuffer(pEngineCore->devices.logical,
                    buffers.vertex.bufferData.buffer, nullptr);
    vkFreeMemory(pEngineCore->devices.logical,
                 buffers.vertex.bufferData.memory, nullptr);

    // memory property flags
    buffers.vertex.bufferData.memoryPropertyFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    // create buffer and allocate memory and bind together
    buffers.vertex.CreateBuffer(
        pEngineCore->devices.physical, pEngineCore->devices.logical,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vertexBufferSize, "UIVertexBuffer");
    buffers.vertex.AllocateBufferMemory(
        pEngineCore->devices.physical, pEngineCore->devices.logical,
        "UIVertexBufferMemory");
    buffers.vertex.bind(pEngineCore->devices.logical, 0);

    // get vertex count from draw data
    buffers.vertexCount = backends.drawData->TotalVtxCount;

    // map memory
    vkMapMemory(pEngineCore->devices.logical,
                buffers.vertex.bufferData.memory, 0,
                vertexBufferSize, 0,
                &buffers.vertex.bufferData.mapped);
  }

  // update index buffer
  if ((buffers.index.bufferData.buffer == VK_NULL_HANDLE) ||
      (buffers.indexCount < backends.drawData->TotalIdxCount)) {

    // unmap
    if (buffers.index.bufferData.mapped != nullptr) {
      vkUnmapMemory(pEngineCore->devices.logical,
                    buffers.index.bufferData.memory);
      buffers.index.bufferData.mapped = nullptr;
    }

    // destroy buffer and allocated memory
    vkDestroyBuffer(pEngineCore->devices.logical,
                    buffers.index.bufferData.buffer, nullptr);
    vkFreeMemory(pEngineCore->devices.logical,
                 buffers.index.bufferData.memory, nullptr);

    // index buffer memory property flags
    buffers.index.bufferData.memoryPropertyFlags =
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

    // create buffer and allocate memory and bind
    buffers.index.CreateBuffer(
        pEngineCore->devices.physical, pEngineCore->devices.logical,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, indexBufferSize, "UIIndexBuffer");
    buffers.index.AllocateBufferMemory(
        pEngineCore->devices.physical, pEngineCore->devices.logical,
        "UIIndexBufferMemory");
    buffers.index.bind(pEngineCore->devices.logical, 0);

    // get index count from draw data
    buffers.indexCount = backends.drawData->TotalIdxCount;

    // map memory
    vkMapMemory(pEngineCore->devices.logical,
                buffers.index.bufferData.memory, 0,
                indexBufferSize, 0,
                &buffers.index.bufferData.mapped);
  }

  // upload data
  ImDrawVert *vtxDst =
      (ImDrawVert *)buffers.vertex.bufferData.mapped;
  ImDrawIdx *idxDst =
      (ImDrawIdx *)buffers.index.bufferData.mapped;

  // copy data to buffer memory
  for (int n = 0; n < backends.drawData->CmdListsCount; n++) {
    const ImDrawList *cmd_list = backends.drawData->CmdLists[n];
    memcpy(vtxDst, cmd_list->VtxBuffer.Data,
           cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
    memcpy(idxDst, cmd_list->IdxBuffer.Data,
           cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
    vtxDst += cmd_list->VtxBuffer.Size;
    idxDst += cmd_list->IdxBuffer.Size;
  }

  // flush buffers to GPU
  if (buffers.vertex.flush(pEngineCore->devices.logical,
                                         VK_WHOLE_SIZE, 0) != VK_SUCCESS) {
    throw std::invalid_argument(" failed to flush UI vertex buffer!");
  }

  if (buffers.index.flush(pEngineCore->devices.logical,
                                        VK_WHOLE_SIZE, 0) != VK_SUCCESS) {
    throw std::invalid_argument(" failed to flush UI index buffer!");
  }
}

void CoreUI::Input(Utilities_UI::ModelData *pModelData) {

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  backends.io = &ImGui::GetIO();

  // file/menu on top left corner of window
  ImGui::Begin("topFrameBar", 0,
               ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoBackground |
                   ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove);

  ImGui::SetWindowPos(ImVec2(0, 0)); // set position to top left
  ImGui::SetWindowSize(ImVec2(
      static_cast<float>(pEngineCore->swapchainData.swapchainExtent2D.width),
      100)); // set size of top left window

  // top left window menu bar
  // file/close
  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File", true)) {

      if (ImGui::MenuItem("Exit", "Esc")) {
        glfwSetWindowShouldClose(pEngineCore->windowGLFW, true);
      }

      ImGui::EndMenu(); // End "File" drop-down menu
    }

    // Place this line after rendering the menu bar
    // ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("
    // framerate: 000.000 ms/frame ( 0.0 FPS )  ").x);

    // Then render your text
    ImGui::Text("framerate: %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

    ImGui::EndMenuBar(); // End main menu bar
  }

  ImGui::End();

  // floating window
  ImGui::Begin("controls");

  if (ImGui::Button("Delete Model")) {
    this->modelData.deleteModel = true;
  }

  if (ImGui::Button("Open File Dialog")) {
    IGFD::FileDialogConfig config;
    config.path = ".";
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File",
                                            ".cpp,.h,.hpp, .gltf", config);
  }

  // display
  if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
    if (ImGuiFileDialog::Instance()->IsOk()) { // action if OK
      // std::string filePathName =
      // ImGuiFileDialog::Instance()->GetFilePathName(); std::string filePath =
      // ImGuiFileDialog::Instance()->GetCurrentPath(); std::cout <<
      // "filePathName: " << filePath << std::endl;
      //   action
    }

    // close
    ImGuiFileDialog::Instance()->Close();
  }

  if (ImGui::CollapsingHeader("Model Controls")) {

    if (ImGui::BeginCombo(
            "Model", pModelData->modelName[pModelData->modelIndex].c_str())) {

      for (int i = 0; i < pModelData->modelName.size(); ++i) {
        bool isSelected = (pModelData->modelIndex == i);
        if (ImGui::Selectable(pModelData->modelName[i].c_str(), isSelected)) {
          pModelData->modelIndex = i;

          std::cout << "Selected Model: "
                    << pModelData->modelName[pModelData->modelIndex]
                    << std::endl;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    if (ImGui::BeginCombo(
            "Animation",
            pModelData
                ->animationNames[pModelData->modelIndex]
                                [pModelData
                                     ->activeAnimation[pModelData->modelIndex]]
                .c_str())) {

      for (int i = 0;
           i < pModelData->animationNames[pModelData->modelIndex].size(); ++i) {
        bool isSelected =
            (pModelData->activeAnimation[pModelData->modelIndex] == i);
        if (ImGui::Selectable(
                pModelData->animationNames[pModelData->modelIndex][i].c_str(),
                isSelected)) {
          pModelData->activeAnimation[pModelData->modelIndex] = i;

          std::cout
              << "Selected Animation: "
              << pModelData->animationNames[pModelData->modelIndex][i].c_str()
              << std::endl;
          this->modelData.isUpdated = true;
        }

        if (isSelected) {
          ImGui::SetItemDefaultFocus();
        }
      }

      ImGui::EndCombo();
    }

    if (ImGui::CollapsingHeader("Model Transform Values")) {

      qRot = {

          this->modelData.transformValues[this->modelData.modelIndex].rotate.w,
          this->modelData.transformValues[this->modelData.modelIndex].rotate.x,
          this->modelData.transformValues[this->modelData.modelIndex].rotate.y,
          this->modelData.transformValues[this->modelData.modelIndex].rotate.z};

      //////// get/setRotation are helper funcs that you have ideally defined to
      //////// manage your global/member objs
      if (ImGui::gizmo3D("Rotation", qRot, 100,
                         imguiGizmo::mode3Axes | imguiGizmo::cubeAtOrigin)) {
        this->modelData.transformValues[this->modelData.modelIndex].rotate.w =
            qRot.w;
        this->modelData.transformValues[this->modelData.modelIndex].rotate.x =
            qRot.x;
        this->modelData.transformValues[this->modelData.modelIndex].rotate.y =
            qRot.y;
        this->modelData.transformValues[this->modelData.modelIndex].rotate.z =
            qRot.z;

        this->modelData.transformMatrices[this->modelData.modelIndex].rotate =
            glm::mat4(glm::normalize(glm::quat(
                this->modelData.transformValues[this->modelData.modelIndex]
                    .rotate.w,
                this->modelData.transformValues[this->modelData.modelIndex]
                    .rotate.x,
                this->modelData.transformValues[this->modelData.modelIndex]
                    .rotate.y,
                this->modelData.transformValues[this->modelData.modelIndex]
                    .rotate.z)));

        this->modelData.rotateUpdated = true;
      }

      // if (ImGui::SliderFloat4("Rotate",
      //                         (float *)&this->modelData
      //                             .transformValues[this->modelData.modelIndex]
      //                             .rotate,
      //                         -180.0f, 180.0f)) {
      //
      //   this->modelData.rotateUpdated = true;
      // }

      if (ImGui::SliderFloat4("Translate",
                              (float *)&this->modelData
                                  .transformValues[this->modelData.modelIndex]
                                  .translate,
                              -20.0f, 20.0f)) {
        this->modelData.transformMatrices[this->modelData.modelIndex]
            .translate = glm::translate(
            glm::mat4(1.0f),
            glm::vec3(
                this->modelData.transformValues[this->modelData.modelIndex]
                    .translate));
        this->modelData.translateUpdated = true;
      }

      if (ImGui::SliderFloat("Scale",
                             (float *)&this->modelData
                                 .transformValues[this->modelData.modelIndex]
                                 .scale,
                             0.001f, 10.0f)) {
        this->modelData.transformMatrices[this->modelData.modelIndex].scale =
            glm::scale(
                glm::mat4(1.0f),
                glm::vec3(
                    this->modelData.transformValues[this->modelData.modelIndex]
                        .scale));
        this->modelData.scaleUpdated = true;
      }

      if (this->modelData.rotateUpdated || this->modelData.translateUpdated ||
          this->modelData.scaleUpdated) {
        this->modelData.rotateUpdated = false;
        this->modelData.translateUpdated = false;
        this->modelData.scaleUpdated = false;
        this->modelData.isUpdated = true;
        this->modelData.updateBLAS.resize(pModelData->modelName.size());
        this->modelData.updateBLAS[pModelData->modelIndex] = true;
        for (int i = 0; i < this->modelData.updateBLAS.size(); i++) {
          std::cout << "model[" << i
                    << "] update blas: " << this->modelData.updateBLAS[i]
                    << std::endl;
        }
      }
    }
  }

  ImGui::End();

  ImGui::Render();

  backends.drawData = ImGui::GetDrawData();
}

void CoreUI::DrawUI(const VkCommandBuffer commandBuffer, int currentFrame) {

  // set viewport and scissor
  pEngineCore->extent.width =
      pEngineCore->swapchainData.swapchainExtent2D.width;
  pEngineCore->extent.height =
      pEngineCore->swapchainData.swapchainExtent2D.height;

  pEngineCore->viewport.width =
      static_cast<float>(pEngineCore->swapchainData.swapchainExtent2D.width);
  pEngineCore->viewport.height =
      static_cast<float>(pEngineCore->swapchainData.swapchainExtent2D.height);

  pEngineCore->viewport.minDepth = 0.0f;
  pEngineCore->viewport.maxDepth = 1.0f;

  pEngineCore->scissor.extent.width =
      pEngineCore->swapchainData.swapchainExtent2D.width;
  pEngineCore->scissor.extent.height =
      pEngineCore->swapchainData.swapchainExtent2D.height;

  pEngineCore->renderArea = {{0, 0}, pEngineCore->extent};

  vkCmdSetViewport(commandBuffer, 0, 1, &pEngineCore->viewport);

  vkCmdSetScissor(commandBuffer, 0, 1, &pEngineCore->scissor);

  // render area
  VkRect2D renderArea = {{0},
                         {pEngineCore->swapchainData.swapchainExtent2D.width,
                          pEngineCore->swapchainData.swapchainExtent2D.height}};

  // clear values (unused rn - renderpass does not clear attachments)
  pEngineCore->colorClearValue.color = {0.0f, 0.0f, 0.0f, 0.0f};

  std::vector<VkClearValue> clearValue = {
      pEngineCore->colorClearValue,
      // pEngineCore->depthClearValue,
      // pEngineCore->colorClearValue
  };

  // get draw data
  // backends.drawData = ImGui::GetDrawData();
  // backends.io = &ImGui::GetIO();

  // vertex and index offset
  int32_t vertexOffset = 0;
  int32_t indexOffset = 0;

  // return if draw data is NULL
  if ((!backends.drawData) || (backends.drawData->CmdListsCount == 0)) {
    return;
  }

  // UI render pass begin info
  VkRenderPassBeginInfo UIPassBeginInfo = {
      VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      nullptr,
      renderData.renderPass,
      renderData.framebuffer[currentFrame],
      renderArea,
      static_cast<uint32_t>(clearValue.size()),
      clearValue.data()};

  // begin render pass
  vkCmdBeginRenderPass(commandBuffer, &UIPassBeginInfo,
                       VK_SUBPASS_CONTENTS_INLINE);

  // bind pipeline
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    renderData.pipeline);

  // bind descriptor sets
  vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                          renderData.pipelineLayout, 0, 1,
                          &descriptor.descriptorSet, 0, NULL);

  // transform
  pushConstant.scale = glm::vec2(2.0f / backends.io->DisplaySize.x,
                                 2.0f / backends.io->DisplaySize.y);

  pushConstant.translate = glm::vec2(-1.0f);

  // bind push constants
  vkCmdPushConstants(commandBuffer, renderData.pipelineLayout,
                     VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(UIPushConstant),
                     &pushConstant);

  // buffer offsets
  std::vector<VkDeviceSize> offsets = {0};

  // bind vertex and index buffers
  vkCmdBindVertexBuffers(commandBuffer, 0, 1,
                         &buffers.vertex.bufferData.buffer,
                         offsets.data());
  vkCmdBindIndexBuffer(commandBuffer,
                       buffers.index.bufferData.buffer, 0,
                       VK_INDEX_TYPE_UINT16);

  // draw
  for (int32_t i = 0; i < backends.drawData->CmdListsCount; i++) {

    const ImDrawList *cmd_list = backends.drawData->CmdLists[i];

    for (int32_t j = 0; j < cmd_list->CmdBuffer.Size; j++) {

      const ImDrawCmd *pcmd = &cmd_list->CmdBuffer[j];
      VkRect2D scissorRect{};
      scissorRect.offset.x = std::max((int32_t)(pcmd->ClipRect.x), 0);
      scissorRect.offset.y = std::max((int32_t)(pcmd->ClipRect.y), 0);
      scissorRect.extent.width =
          (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
      scissorRect.extent.height =
          (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y);
      vkCmdSetScissor(commandBuffer, 0, 1, &scissorRect);
      vkCmdDrawIndexed(commandBuffer, pcmd->ElemCount, 1, indexOffset,
                       vertexOffset, 0);
      indexOffset += pcmd->ElemCount;
    }
    vertexOffset += cmd_list->VtxBuffer.Size;
  }

  // end render pass
  vkCmdEndRenderPass(commandBuffer);
}

void CoreUI::SetModelData(const Utilities_UI::ModelData *pModelData) {

  // set names
  this->modelData = *pModelData;

  std::cout << "\n UI - SetModelData() model name List:" << std::endl;

  for (int i = 0; i < this->modelData.modelName.size(); i++) {
    std::cout << "\t" << this->modelData.modelName[i] << std::endl;
  }
}

void CoreUI::DestroyUI() {

  // framebuffers
  for (const auto &fbuffs : renderData.framebuffer) {
    vkDestroyFramebuffer(pEngineCore->devices.logical, fbuffs, nullptr);
  }

  // buffers
  //for (int i = 0; i < frame_draws; i++) {
    buffers.vertex.destroy(pEngineCore->devices.logical);
    buffers.index.destroy(pEngineCore->devices.logical);
 // }

  // shader
  shader.DestroyShader();

  // pipeline
  vkDestroyPipelineCache(pEngineCore->devices.logical,
                         this->renderData.pipelineCache, nullptr);
  vkDestroyPipeline(pEngineCore->devices.logical, this->renderData.pipeline,
                    nullptr);
  vkDestroyPipelineLayout(pEngineCore->devices.logical,
                          this->renderData.pipelineLayout, nullptr);

  // render pass
  vkDestroyRenderPass(pEngineCore->devices.logical, this->renderData.renderPass,
                      nullptr);

  // descriptor
  vkDestroyDescriptorSetLayout(pEngineCore->devices.logical,
                               this->descriptor.descriptorSetLayout, nullptr);
  vkDestroyDescriptorPool(pEngineCore->devices.logical,
                          this->descriptor.descriptorPool, nullptr);

  // font image
  vkDestroyImageView(pEngineCore->devices.logical, this->fontImage.view,
                     nullptr);
  vkDestroyImage(pEngineCore->devices.logical, this->fontImage.image, nullptr);
  vkFreeMemory(pEngineCore->devices.logical, this->fontImage.memory, nullptr);
  vkDestroySampler(pEngineCore->devices.logical, this->fontImage.sampler,
                   nullptr);

  ImGui_ImplVulkan_Shutdown();
}
