#include "MainRenderer.hpp"

MainRenderer::MainRenderer() {}

MainRenderer::MainRenderer(EngineCore *coreBase) {

  Init_MainRenderer(coreBase);
}

void MainRenderer::Init_MainRenderer(EngineCore *coreBase) {

  // init core pointer
  this->pEngineCore = coreBase;

  // shader
  shader = gtp::Shader(pEngineCore);

  // load assets
  this->LoadAssets();

  std::cout << "\nfinished loading assets." << std::endl;

  this->PreTransformModel();
  //
  vkDeviceWaitIdle(this->pEngineCore->devices.logical);
  //
  ////compute
  this->gltfCompute = ComputeVertex(pEngineCore, this->assets.animatedModel);
  //
  ////create bottom level acceleration structure
  this->CreateBottomLevelAccelerationStructures();

  std::cout << "\nfinished creating bottom level acceleration structures."
            << std::endl;

  ////create geometry nodes buffer
  this->CreateGeometryNodesBuffer();

  std::cout << "\nfinished creating geometry nodes buffers." << std::endl;

  ////create top level acceleration structure
  this->CreateTLAS();

  std::cout << "\nfinished creating top level acceleration structure"
            << std::endl;

  ////create storage image
  this->CreateStorageImage();

  std::cout << "\nfinished creating storage image" << std::endl;

  // create uniform buffer
  this->CreateUniformBuffer();

  std::cout << "\nfinished creating uniform buffer" << std::endl;

  // create ray tracing pipeline
  this->CreateRayTracingPipeline();

  std::cout << "\nfinished creating ray tracing pipeline" << std::endl;

  ////create shader binding tables
  this->CreateShaderBindingTable();

  std::cout << "\nfinished creating shader binding table" << std::endl;

  // create descriptor set
  this->CreateDescriptorSet();

  std::cout << "\nfinished creating descriptor set" << std::endl;

  // setup buffer region device addresses
  this->SetupBufferRegionAddresses();

  std::cout << "\nfinished setting up buffer region addresses" << std::endl;

  // build command buffers
  this->BuildCommandBuffers();

  std::cout << "\nfinished building command buffers" << std::endl;
}

void MainRenderer::LoadAssets() {

  const uint32_t glTFLoadingFlags =
      gtp::FileLoadingFlags::PreTransformVertices |
      gtp::FileLoadingFlags::PreMultiplyVertexColors;

  // this->assets.animatedModel = new gtp::MainRenderer_model(pEngineCore);
  this->assets.animatedModel = new gtp::Model();

  this->assets.animatedModel->loadFromFile(
      "C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/models/Fox2/"
      "Fox2.gltf",
      pEngineCore, pEngineCore->queue.graphics);

  this->assets.models.push_back(this->assets.animatedModel);

  this->assets.testScene = new gtp::Model();
  this->assets.testScene->loadFromFile(
      "C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/models/"
      "test_scene/testScene.gltf",
      pEngineCore, pEngineCore->queue.graphics);

  this->assets.models.push_back(this->assets.testScene);

  this->assets.waterSurface = new gtp::Model();
  this->assets.waterSurface->loadFromFile(
      "C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/models/"
      "test_scene/pool_water_surface/pool_water_surface.gltf",
      pEngineCore, pEngineCore->queue.graphics, glTFLoadingFlags);

  this->assets.models.push_back(this->assets.waterSurface);

  // std::cout << "this->assets.waterSurface->textures.size(): " <<
  // this->assets.waterSurface.textures.size() << std::endl;

  this->assets.coloredGlassTexture = gtp::TextureLoader(this->pEngineCore);
  this->assets.coloredGlassTexture.loadFromFile(
      "C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/textures/"
      "colored_glass_rgba.ktx",
      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  // this->assets.gondola = new gtp::Model();
  // this->assets.gondola->loadFromFile("C:/Users/akral/vulkan_raytracing/vulkan_raytracing/assets/models/test_scene/testScene.gltf",
  //	pEngineCore, pEngineCore->queue.graphics, glTFLoadingFlags);
  //
  // this->assets.models.push_back(this->assets.gondola);
  //
  // std::cout << "this->assets.gondola->textures.size(): " <<
  // this->assets.gondola->textures.size() << std::endl;
}

void MainRenderer::CreateBottomLevelAccelerationStructures() {

  // texture offset - imperative to index textures via geometries in ray trace
  // shaders
  uint32_t textureOffset = 0;

  // resize bottom level acceleration structures as per loaded models
  this->bottomLevelAccelerationStructures.resize(this->assets.models.size());

  // create bottom level acceleration structures
  for (int i = 0; i < this->bottomLevelAccelerationStructures.size(); i++) {
    Utilities_AS::createBLAS(
        this->pEngineCore, &this->geometryNodeBuf, &this->geometryIndexBuf,
        &this->bottomLevelAccelerationStructures[i],
        &this->bottomLevelAccelerationStructures[i].accelerationStructure,
        this->assets.models[i], textureOffset);

    // increment offset by size of models texture array
    textureOffset +=
        static_cast<uint32_t>(this->assets.models[i]->textures.size());
    std::cout << "textureOffset: " << textureOffset << std::endl;
  }
}

void MainRenderer::CreateTLAS() {

  // -- instances transform matrix
  VkTransformMatrixKHR transformMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f,
                                          0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};

  // -- array of instances
  std::vector<VkAccelerationStructureInstanceKHR> blasInstances;

  // resize array
  blasInstances.resize(this->bottomLevelAccelerationStructures.size());

  // initialize instances array
  for (int i = 0; i < blasInstances.size(); i++) {
    blasInstances[i].transform = transformMatrix;
    blasInstances[i].instanceCustomIndex = i;
    blasInstances[i].mask = 0xFF;
    blasInstances[i].instanceShaderBindingTableRecordOffset = 0;
    blasInstances[i].accelerationStructureReference =
        this->bottomLevelAccelerationStructures[i]
            .accelerationStructure.deviceAddress;
    std::cout << "this->secondBLAS.deviceAddress"
              << this->bottomLevelAccelerationStructures[i]
                     .accelerationStructure.deviceAddress
              << std::endl;
  }

  // -- create instances buffer
  buffers.tlas_instancesBuffer.bufferData.bufferName =
      "MainRenderer_TLASInstancesBuffer";
  buffers.tlas_instancesBuffer.bufferData.bufferMemoryName =
      "MainRenderer_TLASInstancesBufferMemory";

  if (pEngineCore->CreateBuffer(
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &buffers.tlas_instancesBuffer,
          sizeof(VkAccelerationStructureInstanceKHR) *
              static_cast<uint32_t>(blasInstances.size()),
          blasInstances.data()) != VK_SUCCESS) {
    throw std::invalid_argument(
        "failed to create MainRenderer instances buffer");
  }

  // -- instance buffer device address
  tlasData.instanceDataDeviceAddress.deviceAddress =
      Utilities_AS::getBufferDeviceAddress(
          this->pEngineCore, buffers.tlas_instancesBuffer.bufferData.buffer);

  // -- acceleration Structure Geometry{};
  tlasData.accelerationStructureGeometry.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  tlasData.accelerationStructureGeometry.geometryType =
      VK_GEOMETRY_TYPE_INSTANCES_KHR;
  tlasData.accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  tlasData.accelerationStructureGeometry.geometry.instances.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  tlasData.accelerationStructureGeometry.geometry.instances.arrayOfPointers =
      VK_FALSE;
  tlasData.accelerationStructureGeometry.geometry.instances.data =
      tlasData.instanceDataDeviceAddress;

  //  -- Get size info -- //
  // Acceleration Structure Build Geometry Info
  tlasData.accelerationStructureBuildGeometryInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  tlasData.accelerationStructureBuildGeometryInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlasData.accelerationStructureBuildGeometryInfo.flags =
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  tlasData.accelerationStructureBuildGeometryInfo.geometryCount = 1;
  tlasData.accelerationStructureBuildGeometryInfo.pGeometries =
      &tlasData.accelerationStructureGeometry;

  // -- tlas data member
  // primitive count
  tlasData.primitive_count = static_cast<uint32_t>(blasInstances.size());

  // -- acceleration Structure Build Sizes Info
  tlasData.accelerationStructureBuildSizesInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  // -- get acceleration structure build sizes
  pEngineCore->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
      pEngineCore->devices.logical,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &tlasData.accelerationStructureBuildGeometryInfo,
      &tlasData.primitive_count, &tlasData.accelerationStructureBuildSizesInfo);

  // -- create acceleration structure buffer
  Utilities_AS::createAccelerationStructureBuffer(
      this->pEngineCore, &this->TLAS.memory, &this->TLAS.buffer,
      &tlasData.accelerationStructureBuildSizesInfo,
      "glTFAnimation_TLASBuffer");

  // -- acceleration structure create info
  VkAccelerationStructureCreateInfoKHR accelerationStructureCreateInfo{};
  accelerationStructureCreateInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
  accelerationStructureCreateInfo.buffer = this->TLAS.buffer;
  accelerationStructureCreateInfo.size =
      tlasData.accelerationStructureBuildSizesInfo.accelerationStructureSize;
  accelerationStructureCreateInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;

  // -- create acceleration structure
  pEngineCore->add(
      [this, &accelerationStructureCreateInfo]() {
        return pEngineCore->objCreate.VKCreateAccelerationStructureKHR(
            &accelerationStructureCreateInfo, nullptr,
            &TLAS.accelerationStructureKHR);
      },
      "MainRenderer_accelerationStructureKHR");

  // -- create scratch buffer
  Utilities_AS::createScratchBuffer(
      this->pEngineCore, &buffers.tlas_scratch,
      tlasData.accelerationStructureBuildSizesInfo.buildScratchSize,
      "glTFAnimation_ScratchBufferTLAS");

  // acceleration Build Geometry Info{};
  tlasData.accelerationBuildGeometryInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  tlasData.accelerationBuildGeometryInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  tlasData.accelerationBuildGeometryInfo.flags =
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_KHR;
  tlasData.accelerationBuildGeometryInfo.mode =
      VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  tlasData.accelerationBuildGeometryInfo.dstAccelerationStructure =
      this->TLAS.accelerationStructureKHR;
  tlasData.accelerationBuildGeometryInfo.geometryCount = 1;
  tlasData.accelerationBuildGeometryInfo.pGeometries =
      &tlasData.accelerationStructureGeometry;
  tlasData.accelerationBuildGeometryInfo.scratchData.deviceAddress =
      buffers.tlas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

  // acceleration structure build range info
  VkAccelerationStructureBuildRangeInfoKHR
      accelerationStructureBuildRangeInfo{};
  accelerationStructureBuildRangeInfo.primitiveCount = tlasData.primitive_count;
  accelerationStructureBuildRangeInfo.primitiveOffset = 0;
  accelerationStructureBuildRangeInfo.firstVertex = 0;
  accelerationStructureBuildRangeInfo.transformOffset = 0;

  // array of acceleration structure build range info
  tlasData.accelerationBuildStructureRangeInfos = {
      &accelerationStructureBuildRangeInfo};

  // build the acceleration structure on the device via a one-time command
  // buffer submission implementations can support acceleration structure
  // building on host
  // (VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands)
  // device builds are preferred

  // create one time submit command buffer
  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // build acceleration structure/s
  pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
      commandBuffer, 1, &tlasData.accelerationBuildGeometryInfo,
      tlasData.accelerationBuildStructureRangeInfos.data());

  // flush one time submit command buffer
  pEngineCore->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // get acceleration structure device address
  VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
  accelerationDeviceAddressInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  accelerationDeviceAddressInfo.accelerationStructure =
      this->TLAS.accelerationStructureKHR;

  // get and assign tlas acceleration structure device address value
  this->TLAS.deviceAddress =
      pEngineCore->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(
          pEngineCore->devices.logical, &accelerationDeviceAddressInfo);
}

void MainRenderer::CreateStorageImage() {

  Utilities_AS::createStorageImage(this->pEngineCore, &this->storageImage,
                                   "glTFAnimation_storageImage");
}

void MainRenderer::CreateUniformBuffer() {

  buffers.ubo.bufferData.bufferName = "shadowUBOBuffer";
  buffers.ubo.bufferData.bufferMemoryName = "shadowUBOBufferMemory";

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.ubo, sizeof(UniformData),
                                &uniformData) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create shadow uniform buffer!");
  }

  UpdateUniformBuffer(0.0f);
}

void MainRenderer::UpdateUniformBuffer(float deltaTime) {
  float rotationTime = deltaTime * 0.10f;

  // projection matrix
  glm::mat4 proj = glm::perspective(
      glm::radians(pEngineCore->camera->Zoom),
      float(pEngineCore->swapchainData.swapchainExtent2D.width) /
          float(pEngineCore->swapchainData.swapchainExtent2D.height),
      0.1f, 1000.0f);
  proj[1][1] *= -1.0f;
  uniformData.projInverse = glm::inverse(proj);

  // view matrix
  uniformData.viewInverse = glm::inverse(pEngineCore->camera->GetViewMatrix());

  // light position
  // uniformData.lightPos =
  //	glm::vec4(cos(glm::radians(rotationTime * 360.0f)) * 40.0f,
  //		-20.0f + sin(glm::radians(rotationTime * 360.0f)) * 20.0f,
  //		25.0f + sin(glm::radians(rotationTime * 360.0f)) * 5.0f,
  //		0.0f);

  uniformData.lightPos = glm::vec4(0.0f, 10.0f, 10.0f, 0.0f);

  memcpy(buffers.ubo.bufferData.mapped, &uniformData, sizeof(UniformData));
}

void MainRenderer::CreateRayTracingPipeline() {

  uint32_t imageCount{0};
  for (int i = 0; i < assets.models.size(); i++) {
    imageCount += static_cast<uint32_t>(assets.models[i]->textures.size());
  }
  // imageCount =
  // static_cast<uint32_t>(this->assets.animatedModel->textures.size()) +
  //	static_cast<uint32_t>(this->assets.helmetModel->textures.size()) +
  //	static_cast<uint32_t>(this->assets.reflectionSceneModel->textures.size());

  std::cout << "MainRenderer raytracing pipeline_  imagecount: " << imageCount
            << std::endl;

  // acceleration structure layout binding
  VkDescriptorSetLayoutBinding accelerationStructureLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          0, VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 1,
          VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
          nullptr);

  // storage image layout binding
  VkDescriptorSetLayoutBinding storageImageLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
          VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr);

  // uniform buffer layout binding
  VkDescriptorSetLayoutBinding uniformBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
          VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_MISS_BIT_KHR,
          nullptr);

  ////texture image layout binding
  // VkDescriptorSetLayoutBinding textureImageLayoutBinding =
  // gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
  // 3,
  // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
  // VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
  // nullptr);

  ////geometry node layout binding
  // VkDescriptorSetLayoutBinding geometryNodeLayoutBinding =
  // gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
  // 4,
  // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
  // VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr);
  //
  ////second geometry node layout binding
  // VkDescriptorSetLayoutBinding secondGeometryNodeLayoutBinding =
  // gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
  // 5,
  // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
  // VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr);

  // g_node_buffer layout binding
  VkDescriptorSetLayoutBinding g_node_bufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          nullptr);

  // g_node_indices layout binding
  VkDescriptorSetLayoutBinding g_node_indicesLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          nullptr);

  // texture image layout binding
  VkDescriptorSetLayoutBinding glassTextureImagesLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
          nullptr);

  // texture image layout binding
  VkDescriptorSetLayoutBinding allTextureImagesLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
          nullptr);

  // bindings array
  std::vector<VkDescriptorSetLayoutBinding> bindings({
      accelerationStructureLayoutBinding,
      storageImageLayoutBinding,
      uniformBufferLayoutBinding,
      g_node_bufferLayoutBinding,
      g_node_indicesLayoutBinding,
      glassTextureImagesLayoutBinding,
      allTextureImagesLayoutBinding,

  });

  // Unbound set
  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
  setLayoutBindingFlags.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
  setLayoutBindingFlags.bindingCount = 7;
  std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
      0,
      0,
      0,
      0,
      0,
      0,
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};

  setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount =
      static_cast<uint32_t>(bindings.size());
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();
  descriptorSetLayoutCreateInfo.pNext = &setLayoutBindingFlags;

  // create descriptor set layout
  pEngineCore->add(
      [this, &descriptorSetLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorSetLayout(
            &descriptorSetLayoutCreateInfo, nullptr,
            &pipelineData.descriptorSetLayout);
      },
      "glTFAnimation_DescriptorSetLayout");

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

  // create pipeline layout
  pEngineCore->add(
      [this, &pipelineLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreatePipelineLayout(
            &pipelineLayoutCreateInfo, nullptr, &pipelineData.pipelineLayout);
      },
      "gltShadTex_RayTracingPipelineLayout");

  // project directory for loading shader modules
  std::filesystem::path projDirectory = std::filesystem::current_path();

  // Setup ray tracing shader groups
  std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

  VkSpecializationMapEntry specializationMapEntry{};
  specializationMapEntry.constantID = 0;
  specializationMapEntry.offset = 0;
  specializationMapEntry.size = sizeof(uint32_t);

  uint32_t maxRecursion = 4;

  VkSpecializationInfo specializationInfo{};
  specializationInfo.mapEntryCount = 1;
  specializationInfo.pMapEntries = &specializationMapEntry;
  specializationInfo.dataSize = sizeof(maxRecursion);
  specializationInfo.pData = &maxRecursion;

  // Ray generation group
  {
    shaderStages.push_back(shader.loadShader(
        projDirectory.string() +
            "/shaders/compiled/main_renderer_raygen.rgen.spv",
        VK_SHADER_STAGE_RAYGEN_BIT_KHR, "main_renderer_raygen"));
    shaderStages.back().pSpecializationInfo = &specializationInfo;
    VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
    shaderGroup.sType =
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
    shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(shaderGroup);
  }

  // Miss group
  {
    shaderStages.push_back(
        shader.loadShader(projDirectory.string() +
                              "/shaders/compiled/main_renderer_miss.rmiss.spv",
                          VK_SHADER_STAGE_MISS_BIT_KHR, "main_renderer_miss"));
    VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
    shaderGroup.sType =
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
    shaderGroup.closestHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.anyHitShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    shaderGroups.push_back(shaderGroup);
    // Second shader for glTFShadows
    shaderStages.push_back(shader.loadShader(
        projDirectory.string() +
            "/shaders/compiled/main_renderer_shadow.rmiss.spv",
        VK_SHADER_STAGE_MISS_BIT_KHR, "main_renderer_shadowmiss"));
    shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
    shaderGroups.push_back(shaderGroup);
  }

  // Closest hit group for doing texture lookups
  {
    shaderStages.push_back(shader.loadShader(
        projDirectory.string() +
            "/shaders/compiled/main_renderer_closesthit.rchit.spv",
        VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR, "main_renderer_closestHit"));
    VkRayTracingShaderGroupCreateInfoKHR shaderGroup{};
    shaderGroup.sType =
        VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    shaderGroup.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    shaderGroup.generalShader = VK_SHADER_UNUSED_KHR;
    shaderGroup.closestHitShader =
        static_cast<uint32_t>(shaderStages.size()) - 1;
    shaderGroup.intersectionShader = VK_SHADER_UNUSED_KHR;
    // This group also uses an anyhit shader for doing transparency (see
    // anyhit.rahit for details)
    shaderStages.push_back(shader.loadShader(
        projDirectory.string() +
            "/shaders/compiled/main_renderer_anyhit.rahit.spv",
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "MainRenderer_anyhit"));
    shaderGroup.anyHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
    shaderGroups.push_back(shaderGroup);
  }

  //	Create the ray tracing pipeline
  VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
  rayTracingPipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  rayTracingPipelineCreateInfo.stageCount =
      static_cast<uint32_t>(shaderStages.size());
  rayTracingPipelineCreateInfo.pStages = shaderStages.data();
  rayTracingPipelineCreateInfo.groupCount =
      static_cast<uint32_t>(shaderGroups.size());
  rayTracingPipelineCreateInfo.pGroups = shaderGroups.data();
  rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
  rayTracingPipelineCreateInfo.layout = pipelineData.pipelineLayout;

  // create raytracing pipeline
  pEngineCore->add(
      [this, &rayTracingPipelineCreateInfo]() {
        return pEngineCore->objCreate.VKCreateRaytracingPipeline(
            &rayTracingPipelineCreateInfo, nullptr, &pipelineData.pipeline);
      },
      "glTFAnimation_RaytracingPipeline");
}

void MainRenderer::CreateShaderBindingTable() {
  // handle size
  const uint32_t handleSize =
      pEngineCore->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize;

  // aligned handle size
  const uint32_t handleSizeAligned = gtp::Utilities_EngCore::alignedSize(
      pEngineCore->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
      pEngineCore->deviceProperties.rayTracingPipelineKHR
          .shaderGroupHandleAlignment);

  // group count
  const uint32_t groupCount = static_cast<uint32_t>(shaderGroups.size());

  // total SBT size
  const uint32_t sbtSize = groupCount * handleSizeAligned;

  // shader handle storage
  std::vector<uint8_t> shaderHandleStorage(sbtSize);

  // get ray tracing shader handle group sizes
  if (pEngineCore->coreExtensions->vkGetRayTracingShaderGroupHandlesKHR(
          pEngineCore->devices.logical, pipelineData.pipeline, 0, groupCount,
          sbtSize, shaderHandleStorage.data()) != VK_SUCCESS) {
    throw std::invalid_argument(
        "failed to get ray tracing shader group handle sizes");
  }

  // buffer usage flags
  const VkBufferUsageFlags bufferUsageFlags =
      VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

  // memory usage flags
  const VkMemoryPropertyFlags memoryUsageFlags =
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

  try {
    // create buffers
    // raygen
    raygenShaderBindingTable.bufferData.bufferName =
        "shadowRaygenShaderBindingTable";
    raygenShaderBindingTable.bufferData.bufferMemoryName =
        "shadowRaygenShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(bufferUsageFlags, memoryUsageFlags,
                                  &raygenShaderBindingTable, handleSize,
                                  nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create raygenShaderBindingTable");
    }

    // miss
    missShaderBindingTable.bufferData.bufferName =
        "shadowMissShaderBindingTable";
    missShaderBindingTable.bufferData.bufferMemoryName =
        "shadowMissShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(bufferUsageFlags, memoryUsageFlags,
                                  &missShaderBindingTable, handleSize * 2,
                                  nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create missShaderBindingTable");
    }

    // hit
    hitShaderBindingTable.bufferData.bufferName = "shadowHitShaderBindingTable";
    hitShaderBindingTable.bufferData.bufferMemoryName =
        "shadowHitShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(bufferUsageFlags, memoryUsageFlags,
                                  &hitShaderBindingTable, handleSize * 2,
                                  nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create hitShaderBindingTable");
    }

    // copy buffers
    raygenShaderBindingTable.map(pEngineCore->devices.logical);
    memcpy(raygenShaderBindingTable.bufferData.mapped,
           shaderHandleStorage.data(), handleSize);
    raygenShaderBindingTable.unmap(pEngineCore->devices.logical);

    missShaderBindingTable.map(pEngineCore->devices.logical);
    memcpy(missShaderBindingTable.bufferData.mapped,
           shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
    missShaderBindingTable.unmap(pEngineCore->devices.logical);

    hitShaderBindingTable.map(pEngineCore->devices.logical);
    memcpy(hitShaderBindingTable.bufferData.mapped,
           shaderHandleStorage.data() + handleSizeAligned * 3, handleSize * 2);
    hitShaderBindingTable.unmap(pEngineCore->devices.logical);
  }

  catch (const std::exception &e) {
    std::cerr << "Error creating shader binding table: " << e.what()
              << std::endl;
    // Handle error (e.g., cleanup resources, rethrow, etc.)
    // You may want to add additional error handling here based on your
    // application's requirements
    throw;
  }
}

void MainRenderer::CreateDescriptorSet() {

  // image count
  uint32_t imageCount{0};
  for (int i = 0; i < assets.models.size(); i++) {
    imageCount += static_cast<uint32_t>(assets.models[i]->textures.size());
  }

  // std::cout <<
  // "!!!!!!!!!!!CREATEDESCRIPTORSETS!!!!!!!!!!!\nstatic_cast<uint32_t>(this->assets.animatedModel->textures.size():
  // "
  //	<< static_cast<uint32_t>(this->assets.animatedModel->textures.size()) <<
  // std::endl; descriptor pool sizes
  std::vector<VkDescriptorPoolSize> poolSizes = {
      {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 150},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10},
  };
  // descriptor pool create info
  VkDescriptorPoolCreateInfo descriptorPoolCreateInfo{};
  descriptorPoolCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  descriptorPoolCreateInfo.poolSizeCount =
      static_cast<uint32_t>(poolSizes.size());
  descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
  descriptorPoolCreateInfo.maxSets = 1;

  // create descriptor pool
  pEngineCore->add(
      [this, &descriptorPoolCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorPool(
            &descriptorPoolCreateInfo, nullptr,
            &this->pipelineData.descriptorPool);
      },
      "glTFAnimation_DescriptorPool");

  VkDescriptorSetVariableDescriptorCountAllocateInfoEXT
      variableDescriptorCountAllocInfo{};
  uint32_t variableDescCounts[] = {imageCount};
  variableDescriptorCountAllocInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO_EXT;
  variableDescriptorCountAllocInfo.descriptorSetCount = 1;
  variableDescriptorCountAllocInfo.pDescriptorCounts = variableDescCounts;

  VkDescriptorSetAllocateInfo descriptorSetAllocateInfo{};
  descriptorSetAllocateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  descriptorSetAllocateInfo.descriptorPool = this->pipelineData.descriptorPool;
  descriptorSetAllocateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;
  descriptorSetAllocateInfo.descriptorSetCount = 1;
  descriptorSetAllocateInfo.pNext = &variableDescriptorCountAllocInfo;

  // create descriptor set
  pEngineCore->add(
      [this, &descriptorSetAllocateInfo]() {
        return pEngineCore->objCreate.VKAllocateDescriptorSet(
            &descriptorSetAllocateInfo, nullptr,
            &this->pipelineData.descriptorSet);
      },
      "glTFAnimation_DescriptorSet");

  VkWriteDescriptorSetAccelerationStructureKHR
      descriptorAccelerationStructureInfo{};
  descriptorAccelerationStructureInfo.sType =
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
  descriptorAccelerationStructureInfo.pAccelerationStructures =
      &this->TLAS.accelerationStructureKHR;

  VkWriteDescriptorSet accelerationStructureWrite{};
  accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  // The specialized acceleration structure descriptor has to be chained
  accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
  accelerationStructureWrite.dstSet = pipelineData.descriptorSet;
  accelerationStructureWrite.dstBinding = 0;
  accelerationStructureWrite.descriptorCount = 1;
  accelerationStructureWrite.descriptorType =
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

  VkDescriptorImageInfo storageImageDescriptor{
      VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL};

  // storage/result image write
  VkWriteDescriptorSet storageImageWrite{};
  storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageImageWrite.dstSet = pipelineData.descriptorSet;
  storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  storageImageWrite.dstBinding = 1;
  storageImageWrite.pImageInfo = &storageImageDescriptor;
  storageImageWrite.descriptorCount = 1;

  // ubo descriptor info
  VkDescriptorBufferInfo uboDescriptor{};
  uboDescriptor.buffer = buffers.ubo.bufferData.buffer;
  uboDescriptor.offset = 0;
  uboDescriptor.range = buffers.ubo.bufferData.size;

  // ubo descriptor write info
  VkWriteDescriptorSet uniformBufferWrite{};
  uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  uniformBufferWrite.dstSet = pipelineData.descriptorSet;
  uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformBufferWrite.dstBinding = 2;
  uniformBufferWrite.pBufferInfo = &uboDescriptor;
  uniformBufferWrite.descriptorCount = 1;

  // g_nodes_buffer
  VkDescriptorBufferInfo g_nodes_BufferDescriptor{
      this->buffers.g_nodes_buffer.bufferData.buffer, 0,
      this->buffers.g_nodes_buffer.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet g_nodes_bufferWrite{};
  g_nodes_bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  g_nodes_bufferWrite.dstSet = pipelineData.descriptorSet;
  g_nodes_bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  g_nodes_bufferWrite.dstBinding = 3;
  g_nodes_bufferWrite.pBufferInfo = &g_nodes_BufferDescriptor;
  g_nodes_bufferWrite.descriptorCount = 1;

  // g_nodes_indices
  VkDescriptorBufferInfo g_nodes_indicesDescriptor{
      this->buffers.g_nodes_indices.bufferData.buffer, 0,
      this->buffers.g_nodes_indices.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet g_nodes_indicesWrite{};
  g_nodes_indicesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  g_nodes_indicesWrite.dstSet = pipelineData.descriptorSet;
  g_nodes_indicesWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  g_nodes_indicesWrite.dstBinding = 4;
  g_nodes_indicesWrite.pBufferInfo = &g_nodes_indicesDescriptor;
  g_nodes_indicesWrite.descriptorCount = 1;

  VkDescriptorImageInfo glassTextureDescriptor{
      this->assets.coloredGlassTexture.sampler,
      this->assets.coloredGlassTexture.view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  // storage/result image write
  VkWriteDescriptorSet glassTextureWrite{};
  glassTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  glassTextureWrite.dstSet = pipelineData.descriptorSet;
  glassTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glassTextureWrite.dstBinding = 5;
  glassTextureWrite.pImageInfo = &glassTextureDescriptor;
  glassTextureWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 0: Top level acceleration structure
      accelerationStructureWrite,
      // Binding 1: Ray tracing result image
      storageImageWrite,
      // Binding 2: Uniform data
      uniformBufferWrite,
      // Binding 3: g_nodes_buffer write
      g_nodes_bufferWrite,
      // Binding 4: g_nodes_indices write
      g_nodes_indicesWrite,
      // Binding 5: glass texture image write
      glassTextureWrite};

  // Image descriptors for the image array
  std::vector<VkDescriptorImageInfo> textureDescriptors{};

  for (int i = 0; i < assets.models.size(); i++) {
    for (int j = 0; j < assets.models[i]->textures.size(); j++) {
      VkDescriptorImageInfo descriptor{};
      descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      descriptor.sampler = assets.models[i]->textures[j].sampler;
      descriptor.imageView = assets.models[i]->textures[j].view;
      textureDescriptors.push_back(descriptor);
    }
  }

  VkWriteDescriptorSet writeDescriptorImgArray{};
  writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorImgArray.dstBinding = 6;
  writeDescriptorImgArray.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorImgArray.descriptorCount =
      static_cast<uint32_t>(textureDescriptors.size());
  writeDescriptorImgArray.dstSet = this->pipelineData.descriptorSet;
  writeDescriptorImgArray.pImageInfo = textureDescriptors.data();
  writeDescriptorSets.push_back(writeDescriptorImgArray);

  vkUpdateDescriptorSets(this->pEngineCore->devices.logical,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void MainRenderer::SetupBufferRegionAddresses() {

  // setup buffer regions pointing to shaders in shader binding table
  const uint32_t handleSizeAligned = gtp::Utilities_EngCore::alignedSize(
      pEngineCore->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
      pEngineCore->deviceProperties.rayTracingPipelineKHR
          .shaderGroupHandleAlignment);

  // VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
  raygenStridedDeviceAddressRegion.deviceAddress =
      Utilities_AS::getBufferDeviceAddress(
          this->pEngineCore, raygenShaderBindingTable.bufferData.buffer);
  raygenStridedDeviceAddressRegion.stride = handleSizeAligned;
  raygenStridedDeviceAddressRegion.size = handleSizeAligned;

  // VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
  missStridedDeviceAddressRegion.deviceAddress =
      Utilities_AS::getBufferDeviceAddress(
          this->pEngineCore, missShaderBindingTable.bufferData.buffer);
  missStridedDeviceAddressRegion.stride = handleSizeAligned;
  missStridedDeviceAddressRegion.size = handleSizeAligned * 2;

  // VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
  hitStridedDeviceAddressRegion.deviceAddress =
      Utilities_AS::getBufferDeviceAddress(
          this->pEngineCore, hitShaderBindingTable.bufferData.buffer);
  hitStridedDeviceAddressRegion.stride = handleSizeAligned;
  hitStridedDeviceAddressRegion.size = handleSizeAligned * 2;
}

void MainRenderer::BuildCommandBuffers() {

  // if (resized)
  //{
  //	handleResize();
  // }

  VkCommandBufferBeginInfo cmdBufInfo{};
  cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1,
                                              0, 1};

  for (int32_t i = 0; i < pEngineCore->commandBuffers.graphics.size(); ++i) {

    // std::cout << " command buffers [" << i << "]" << std::endl;

    if (vkBeginCommandBuffer(pEngineCore->commandBuffers.graphics[i],
                             &cmdBufInfo) != VK_SUCCESS) {
      throw std::invalid_argument(
          "failed to begin recording graphics command buffer");
    }

    // dispatch the ray tracing commands
    vkCmdBindPipeline(pEngineCore->commandBuffers.graphics[i],
                      VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                      pipelineData.pipeline);

    vkCmdBindDescriptorSets(pEngineCore->commandBuffers.graphics[i],
                            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                            pipelineData.pipelineLayout, 0, 1,
                            &pipelineData.descriptorSet, 0, nullptr);

    VkStridedDeviceAddressRegionKHR emptyShaderSbtEntry{};
    pEngineCore->coreExtensions->vkCmdTraceRaysKHR(
        pEngineCore->commandBuffers.graphics[i],
        &raygenStridedDeviceAddressRegion, &missStridedDeviceAddressRegion,
        &hitStridedDeviceAddressRegion, &emptyShaderSbtEntry,
        pEngineCore->swapchainData.swapchainExtent2D.width,
        pEngineCore->swapchainData.swapchainExtent2D.height, 1);

    // copy ray tracing output to swap chain image
    // prepare current swap chain image as transfer destination
    gtp::Utilities_EngCore::setImageLayout(
        pEngineCore->commandBuffers.graphics[i],
        pEngineCore->swapchainData.swapchainImages.image[i],
        VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        subresourceRange);

    // prepare ray tracing output image as transfer source
    gtp::Utilities_EngCore::setImageLayout(
        pEngineCore->commandBuffers.graphics[i], storageImage.image,
        VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        subresourceRange);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {pEngineCore->swapchainData.swapchainExtent2D.width,
                         pEngineCore->swapchainData.swapchainExtent2D.height,
                         1};

    vkCmdCopyImage(pEngineCore->commandBuffers.graphics[i], storageImage.image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   pEngineCore->swapchainData.swapchainImages.image[i],
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

    // transition swap chain image back for presentation
    gtp::Utilities_EngCore::setImageLayout(
        pEngineCore->commandBuffers.graphics[i],
        pEngineCore->swapchainData.swapchainImages.image[i],
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        subresourceRange);

    // transition ray tracing output image back to general layout
    gtp::Utilities_EngCore::setImageLayout(
        pEngineCore->commandBuffers.graphics[i], storageImage.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange);

    if (vkEndCommandBuffer(pEngineCore->commandBuffers.graphics[i]) !=
        VK_SUCCESS) {
      throw std::invalid_argument("failed to end recording command buffer");
    }
  }
}

void MainRenderer::RebuildCommandBuffers(int frame) {

  // subresource range
  VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1,
                                              0, 1};

  VkStridedDeviceAddressRegionKHR emptySbtEntry{};

  // dispatch the ray tracing commands
  vkCmdBindPipeline(pEngineCore->commandBuffers.graphics[frame],
                    VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                    pipelineData.pipeline);

  vkCmdBindDescriptorSets(pEngineCore->commandBuffers.graphics[frame],
                          VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
                          pipelineData.pipelineLayout, 0, 1,
                          &pipelineData.descriptorSet, 0, 0);

  pEngineCore->coreExtensions->vkCmdTraceRaysKHR(
      pEngineCore->commandBuffers.graphics[frame],
      &raygenStridedDeviceAddressRegion, &missStridedDeviceAddressRegion,
      &hitStridedDeviceAddressRegion, &emptySbtEntry,
      pEngineCore->swapchainData.swapchainExtent2D.width,
      pEngineCore->swapchainData.swapchainExtent2D.height, 1);

  // copy ray tracing output to swap chain image
  // prepare current swap chain image as transfer destination
  gtp::Utilities_EngCore::setImageLayout(
      pEngineCore->commandBuffers.graphics[frame],
      pEngineCore->swapchainData.swapchainImages.image[frame],
      VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      subresourceRange);

  ////std::cout << "test"<< std::endl;
  // prepare ray tracing output image as transfer source

  gtp::Utilities_EngCore::setImageLayout(
      pEngineCore->commandBuffers.graphics[frame], storageImage.image,
      VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      subresourceRange);

  VkImageCopy copyRegion{};
  copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  copyRegion.srcOffset = {0, 0, 0};
  copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  copyRegion.dstOffset = {0, 0, 0};
  copyRegion.extent = {pEngineCore->swapchainData.swapchainExtent2D.width,
                       pEngineCore->swapchainData.swapchainExtent2D.height, 1};

  vkCmdCopyImage(pEngineCore->commandBuffers.graphics[frame],
                 storageImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                 pEngineCore->swapchainData.swapchainImages.image[frame],
                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

  // transition swap chain image back for presentation
  gtp::Utilities_EngCore::setImageLayout(
      pEngineCore->commandBuffers.graphics[frame],
      pEngineCore->swapchainData.swapchainImages.image[frame],
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      subresourceRange);

  // transition ray tracing output image back to general layout
  gtp::Utilities_EngCore::setImageLayout(
      pEngineCore->commandBuffers.graphics[frame], storageImage.image,
      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
      subresourceRange);
}

void MainRenderer::UpdateBLAS() {
  // Build
  // via a one-time command buffer submission
  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // build BLAS
  pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
      commandBuffer, 1,
      &this->bottomLevelAccelerationStructures[0]
           .accelerationStructureBuildGeometryInfo,
      this->bottomLevelAccelerationStructures[0].pBuildRangeInfos.data());

  // end and submit and destroy command buffer
  pEngineCore->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // std::cout << "this->BLAS.deviceAddress" << this->BLAS.deviceAddress <<
  // std::endl;
}

void MainRenderer::UpdateTLAS() {

  VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
  accelerationStructureGeometry.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
  accelerationStructureGeometry.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR;
  accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
  accelerationStructureGeometry.geometry.instances.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
  accelerationStructureGeometry.geometry.instances.arrayOfPointers = VK_FALSE;
  accelerationStructureGeometry.geometry.instances.data =
      tlasData.instanceDataDeviceAddress;

  // Get size info
  /*
  The pSrcAccelerationStructure, dstAccelerationStructure, and mode members of
  pBuildInfo are ignored. Any VkDeviceOrHostAddressKHR members of pBuildInfo are
  ignored by this command, except that the hostAddress member of
  VkAccelerationStructureGeometryTrianglesDataKHR::transformData will be
  examined to check if it is NULL.*
  */
  // VkAccelerationStructureBuildGeometryInfoKHR
  // accelerationStructureBuildGeometryInfo{};
  // accelerationStructureBuildGeometryInfo.sType =
  // VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  // accelerationStructureBuildGeometryInfo.type =
  // VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  // accelerationStructureBuildGeometryInfo.flags =
  // VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  // accelerationStructureBuildGeometryInfo.geometryCount = 1;
  // accelerationStructureBuildGeometryInfo.pGeometries =
  // &accelerationStructureGeometry;

  VkAccelerationStructureBuildGeometryInfoKHR
      accelerationStructureBuildGeometryInfo{};
  accelerationStructureBuildGeometryInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
  accelerationStructureBuildGeometryInfo.type =
      VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  accelerationStructureBuildGeometryInfo.flags =
      VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
  accelerationStructureBuildGeometryInfo.mode =
      VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
  accelerationStructureBuildGeometryInfo.dstAccelerationStructure =
      this->TLAS.accelerationStructureKHR;
  accelerationStructureBuildGeometryInfo.geometryCount = 1;
  accelerationStructureBuildGeometryInfo.pGeometries =
      &accelerationStructureGeometry;
  accelerationStructureBuildGeometryInfo.scratchData.deviceAddress =
      buffers.tlas_scratch.bufferData.bufferDeviceAddress.deviceAddress;

  // uint32_t primitive_count = buffers.tlas_instancesBuffer.;

  VkAccelerationStructureBuildSizesInfoKHR
      accelerationStructureBuildSizesInfo{};
  accelerationStructureBuildSizesInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;

  pEngineCore->coreExtensions->vkGetAccelerationStructureBuildSizesKHR(
      pEngineCore->devices.logical,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &accelerationStructureBuildGeometryInfo, &tlasData.primitive_count,
      &accelerationStructureBuildSizesInfo);

  VkAccelerationStructureBuildRangeInfoKHR
      accelerationStructureBuildRangeInfo{};
  accelerationStructureBuildRangeInfo.primitiveCount = tlasData.primitive_count;
  accelerationStructureBuildRangeInfo.primitiveOffset = 0;
  accelerationStructureBuildRangeInfo.firstVertex = 0;
  accelerationStructureBuildRangeInfo.transformOffset = 0;

  std::vector<VkAccelerationStructureBuildRangeInfoKHR *>
      accelerationBuildStructureRangeInfos = {
          &accelerationStructureBuildRangeInfo};

  // build the acceleration structure on the device via a one-time command
  // buffer submission some implementations may support acceleration structure
  // building on the host
  //(VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands),
  // but we prefer device builds create command buffer
  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

  // this is where i start next
  // build acceleration structures
  pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
      commandBuffer, 1, &accelerationStructureBuildGeometryInfo,
      accelerationBuildStructureRangeInfos.data());

  // flush command buffer
  pEngineCore->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // get acceleration structure device address
  VkAccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo{};
  accelerationDeviceAddressInfo.sType =
      VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
  accelerationDeviceAddressInfo.accelerationStructure =
      this->TLAS.accelerationStructureKHR;

  this->TLAS.deviceAddress =
      pEngineCore->coreExtensions->vkGetAccelerationStructureDeviceAddressKHR(
          pEngineCore->devices.logical, &accelerationDeviceAddressInfo);
}

void MainRenderer::PreTransformModel() {

  // pre transform animation model
  // animation uses bone local transform matrices to reposition model vertices
  // update the model transform matrix to apply an initial transform that will
  // remain glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f),
  // glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); glm::mat4
  // translationMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 1.0f,
  // 0.0f)); for (auto& node : this->assets.animatedModel->linearNodes) {
  // if
  //(node->mesh) {
  //		//node->matrix *= glm::transpose(rotationMatrix);
  //		node->matrix *= translationMatrix;
  //
  //	}
  // }

  ////pre transform water surface
  // std::vector<gtp::Model::Vertex> waterSurfaceVerticesBuffer;
  // waterSurfaceVerticesBuffer =
  // Utilities_AS::GetVerticesFromBuffer(this->pEngineCore->devices.logical,
  // this->assets.waterSurface); VkDeviceSize waterSurfaceBufferSize =
  // static_cast<uint32_t>(waterSurfaceVerticesBuffer.size()) *
  // sizeof(gtp::Model::Vertex);
  //
  // std::cout << "waterSurfaceVerticesBuffer.size(): " <<
  // waterSurfaceVerticesBuffer.size() << std::endl;
  //
  // for (int i = 0; i < waterSurfaceVerticesBuffer.size(); i++) {
  //	waterSurfaceVerticesBuffer[i].pos.y -= 4.0f;
  // }
  //
  // void* waterSurfaceVerticesData;
  //
  // vkMapMemory(this->pEngineCore->devices.logical,
  //	this->assets.waterSurface->vertices.memory, 0,
  //	waterSurfaceBufferSize, 0, &waterSurfaceVerticesData);
  //
  // memcpy(waterSurfaceVerticesData, waterSurfaceVerticesBuffer.data(),
  // waterSurfaceBufferSize);
  //
  // vkUnmapMemory(this->pEngineCore->devices.logical,
  // this->assets.waterSurface->vertices.memory);
  //
  ////pre transform scene
  // std::vector<gtp::Model::Vertex> tempSceneVerticesBuffer;
  // tempSceneVerticesBuffer =
  // Utilities_AS::GetVerticesFromBuffer(this->pEngineCore->devices.logical,
  // this->assets.testScene); VkDeviceSize tempSceneBufferSize =
  // static_cast<uint32_t>(tempSceneVerticesBuffer.size()) *
  // sizeof(gtp::Model::Vertex);
  //
  // std::cout << "tempSceneVerticesBuffer.size(): " <<
  // tempSceneVerticesBuffer.size() << std::endl;
  //
  // for (int i = 0; i < tempSceneVerticesBuffer.size(); i++) {
  //	tempSceneVerticesBuffer[i].pos.y -= 4.0f;
  // }
  //
  // void* verticesData;
  //
  // vkMapMemory(this->pEngineCore->devices.logical,
  //	this->assets.testScene->vertices.memory, 0,
  //	tempSceneBufferSize, 0, &verticesData);
  //
  // memcpy(verticesData, tempSceneVerticesBuffer.data(), tempSceneBufferSize);
  //
  // vkUnmapMemory(this->pEngineCore->devices.logical,
  // this->assets.testScene->vertices.memory);
}

void MainRenderer::CreateGeometryNodesBuffer() {

  buffers.g_nodes_buffer.bufferData.bufferName = "g_nodes_buffer";
  buffers.g_nodes_buffer.bufferData.bufferMemoryName = "g_nodes_bufferMemory";

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.g_nodes_buffer,
                                static_cast<uint32_t>(geometryNodeBuf.size()) *
                                    sizeof(Utilities_AS::GeometryNode),
                                geometryNodeBuf.data()) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create g_nodes_buffer");
  }

  buffers.g_nodes_indices.bufferData.bufferName = "g_nodes_indicesBuffer";
  buffers.g_nodes_indices.bufferData.bufferMemoryName =
      "g_nodes_indicesBufferMemory";

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.g_nodes_indices,
                                static_cast<uint32_t>(geometryIndexBuf.size()) *
                                    sizeof(int),
                                geometryIndexBuf.data()) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create g_nodes_index buffer");
  }
}

void MainRenderer::UpdateDescriptorSet() {

  // image count
  uint32_t imageCount{0};
  for (int i = 0; i < assets.models.size(); i++) {
    imageCount += static_cast<uint32_t>(assets.models[i]->textures.size());
  }

  VkWriteDescriptorSetAccelerationStructureKHR
      descriptorAccelerationStructureInfo{};
  descriptorAccelerationStructureInfo.sType =
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
  descriptorAccelerationStructureInfo.pAccelerationStructures =
      &this->TLAS.accelerationStructureKHR;

  VkWriteDescriptorSet accelerationStructureWrite{};
  accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  // The specialized acceleration structure descriptor has to be chained
  accelerationStructureWrite.pNext = &descriptorAccelerationStructureInfo;
  accelerationStructureWrite.dstSet = pipelineData.descriptorSet;
  accelerationStructureWrite.dstBinding = 0;
  accelerationStructureWrite.descriptorCount = 1;
  accelerationStructureWrite.descriptorType =
      VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;

  VkDescriptorImageInfo storageImageDescriptor{
      VK_NULL_HANDLE, storageImage.view, VK_IMAGE_LAYOUT_GENERAL};

  // storage/result image write
  VkWriteDescriptorSet storageImageWrite{};
  storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageImageWrite.dstSet = pipelineData.descriptorSet;
  storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  storageImageWrite.dstBinding = 1;
  storageImageWrite.pImageInfo = &storageImageDescriptor;
  storageImageWrite.descriptorCount = 1;

  // ubo descriptor info
  VkDescriptorBufferInfo uboDescriptor{};
  uboDescriptor.buffer = buffers.ubo.bufferData.buffer;
  uboDescriptor.offset = 0;
  uboDescriptor.range = buffers.ubo.bufferData.size;

  // ubo descriptor write info
  VkWriteDescriptorSet uniformBufferWrite{};
  uniformBufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  uniformBufferWrite.dstSet = pipelineData.descriptorSet;
  uniformBufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uniformBufferWrite.dstBinding = 2;
  uniformBufferWrite.pBufferInfo = &uboDescriptor;
  uniformBufferWrite.descriptorCount = 1;

  // g_nodes_buffer
  VkDescriptorBufferInfo g_nodes_BufferDescriptor{
      this->buffers.g_nodes_buffer.bufferData.buffer, 0,
      this->buffers.g_nodes_buffer.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet g_nodes_bufferWrite{};
  g_nodes_bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  g_nodes_bufferWrite.dstSet = pipelineData.descriptorSet;
  g_nodes_bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  g_nodes_bufferWrite.dstBinding = 3;
  g_nodes_bufferWrite.pBufferInfo = &g_nodes_BufferDescriptor;
  g_nodes_bufferWrite.descriptorCount = 1;

  // g_nodes_indices
  VkDescriptorBufferInfo g_nodes_indicesDescriptor{
      this->buffers.g_nodes_indices.bufferData.buffer, 0,
      this->buffers.g_nodes_indices.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet g_nodes_indicesWrite{};
  g_nodes_indicesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  g_nodes_indicesWrite.dstSet = pipelineData.descriptorSet;
  g_nodes_indicesWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  g_nodes_indicesWrite.dstBinding = 4;
  g_nodes_indicesWrite.pBufferInfo = &g_nodes_indicesDescriptor;
  g_nodes_indicesWrite.descriptorCount = 1;

  VkDescriptorImageInfo glassTextureDescriptor{
      this->assets.coloredGlassTexture.sampler,
      this->assets.coloredGlassTexture.view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  // storage/result image write
  VkWriteDescriptorSet glassTextureWrite{};
  glassTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  glassTextureWrite.dstSet = pipelineData.descriptorSet;
  glassTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glassTextureWrite.dstBinding = 5;
  glassTextureWrite.pImageInfo = &glassTextureDescriptor;
  glassTextureWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 0: Top level acceleration structure
      accelerationStructureWrite,
      // Binding 1: Ray tracing result image
      storageImageWrite,
      // Binding 2: Uniform data
      uniformBufferWrite,
      // Binding 4: g_nodes_buffer write
      g_nodes_bufferWrite,
      // Binding 5: g_nodes_indices write
      g_nodes_indicesWrite,
      // Binding 5: glass texture write
      glassTextureWrite};

  // Image descriptors for the image array
  std::vector<VkDescriptorImageInfo> textureDescriptors{};

  for (int i = 0; i < assets.models.size(); i++) {
    for (int j = 0; j < assets.models[i]->textures.size(); j++) {
      VkDescriptorImageInfo descriptor{};
      descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      descriptor.sampler = assets.models[i]->textures[j].sampler;
      descriptor.imageView = assets.models[i]->textures[j].view;
      textureDescriptors.push_back(descriptor);
    }
  }

  VkWriteDescriptorSet writeDescriptorImgArray{};
  writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorImgArray.dstBinding = 6;
  writeDescriptorImgArray.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  writeDescriptorImgArray.descriptorCount =
      static_cast<uint32_t>(textureDescriptors.size());
  writeDescriptorImgArray.dstSet = this->pipelineData.descriptorSet;
  writeDescriptorImgArray.pImageInfo = textureDescriptors.data();
  writeDescriptorSets.push_back(writeDescriptorImgArray);

  vkUpdateDescriptorSets(this->pEngineCore->devices.logical,
                         static_cast<uint32_t>(writeDescriptorSets.size()),
                         writeDescriptorSets.data(), 0, VK_NULL_HANDLE);
}

void MainRenderer::HandleResize() {

  // Delete allocated resources
  vkDestroyImageView(pEngineCore->devices.logical, storageImage.view, nullptr);
  vkDestroyImage(pEngineCore->devices.logical, storageImage.image, nullptr);
  vkFreeMemory(pEngineCore->devices.logical, storageImage.memory, nullptr);

  // Recreate image
  this->CreateStorageImage();

  // Update descriptor
  this->UpdateDescriptorSet();
}

void MainRenderer::Destroy_MainRenderer() {

  // -- descriptor pool
  vkDestroyDescriptorPool(pEngineCore->devices.logical,
                          this->pipelineData.descriptorPool, nullptr);

  // -- shader binding tables
  raygenShaderBindingTable.destroy(pEngineCore->devices.logical);
  hitShaderBindingTable.destroy(pEngineCore->devices.logical);
  missShaderBindingTable.destroy(pEngineCore->devices.logical);

  // -- shaders
  shader.DestroyShader();

  // -- pipeline and layout
  vkDestroyPipeline(this->pEngineCore->devices.logical,
                    this->pipelineData.pipeline, nullptr);
  vkDestroyPipelineLayout(this->pEngineCore->devices.logical,
                          this->pipelineData.pipelineLayout, nullptr);

  // -- descriptor set layout
  vkDestroyDescriptorSetLayout(this->pEngineCore->devices.logical,
                               this->pipelineData.descriptorSetLayout, nullptr);

  // -- uniform buffer
  buffers.ubo.destroy(this->pEngineCore->devices.logical);

  // -- storage image
  vkDestroyImageView(pEngineCore->devices.logical, this->storageImage.view,
                     nullptr);
  vkDestroyImage(pEngineCore->devices.logical, this->storageImage.image,
                 nullptr);
  vkFreeMemory(pEngineCore->devices.logical, this->storageImage.memory,
               nullptr);

  // g node buffer
  this->buffers.g_nodes_buffer.destroy(this->pEngineCore->devices.logical);

  // g node indices buffer
  this->buffers.g_nodes_indices.destroy(this->pEngineCore->devices.logical);

  // -- bottom level acceleration structure & related buffers -- //
  for (int i = 0; i < this->bottomLevelAccelerationStructures.size(); i++) {

    // accel. structure
    pEngineCore->coreExtensions->vkDestroyAccelerationStructureKHR(
        pEngineCore->devices.logical,
        this->bottomLevelAccelerationStructures[i]
            .accelerationStructure.accelerationStructureKHR,
        nullptr);

    // scratch buffer
    this->bottomLevelAccelerationStructures[i]
        .accelerationStructure.scratchBuffer.destroy(
            this->pEngineCore->devices.logical);

    // accel structure buffer and memory
    vkDestroyBuffer(
        pEngineCore->devices.logical,
        this->bottomLevelAccelerationStructures[i].accelerationStructure.buffer,
        nullptr);
    vkFreeMemory(
        pEngineCore->devices.logical,
        this->bottomLevelAccelerationStructures[i].accelerationStructure.memory,
        nullptr);
  }

  // -- top level acceleration structure & related buffers -- //

  // accel. structure
  pEngineCore->coreExtensions->vkDestroyAccelerationStructureKHR(
      pEngineCore->devices.logical, this->TLAS.accelerationStructureKHR,
      nullptr);

  // scratch buffer
  buffers.tlas_scratch.destroy(this->pEngineCore->devices.logical);

  // instances buffer
  buffers.tlas_instancesBuffer.destroy(this->pEngineCore->devices.logical);

  // accel. structure buffer and memory
  vkDestroyBuffer(pEngineCore->devices.logical, this->TLAS.buffer, nullptr);
  vkFreeMemory(pEngineCore->devices.logical, this->TLAS.memory, nullptr);

  // transforms buffer
  this->buffers.transformBuffer.destroy(this->pEngineCore->devices.logical);

  // -- compute class
  this->gltfCompute.Destroy_ComputeVertex();

  // -- models
  this->assets.animatedModel->destroy(this->pEngineCore->devices.logical);
  this->assets.testScene->destroy(this->pEngineCore->devices.logical);
  this->assets.waterSurface->destroy(this->pEngineCore->devices.logical);
  this->assets.coloredGlassTexture.DestroyTextureLoader();
  // this->assets.gondola->destroy(this->pEngineCore->devices.logical);
}
