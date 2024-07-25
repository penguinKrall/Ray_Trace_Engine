#include "RenderBase.hpp"

/*  initialize function */
void gtp::RenderBase::InitializeRenderBase(EngineCore *engineCorePtr) {
  // initialize core pointer
  this->pEngineCore = engineCorePtr;

  // initialize tools
  this->tools.InitializeTools(engineCorePtr);

  // load default assets
  this->assets.LoadDefaultAssets(engineCorePtr);

  this->SetTextureOffset(
      static_cast<uint32_t>(this->assets.defaultTextures.size()));

  // create geometry nodes buffer
  this->BuildGeometryNodesBuffer();

  // build top level acceleration structure
  this->BuildTopLevelAccelerationStructure(
      this->assets.models, this->assets.modelData, this->assets.particle);

  // create storage images by initializing StorageImages struct with constructor
  auto uniqueStorageImages =
      std::make_unique<gtp::RenderBase::StorageImages>(this->pEngineCore);
  this->storageImages = uniqueStorageImages.release();

  // create default uniform buffer
  this->CreateDefaultUniformBuffer();

  // create default pipeline
  this->CreateDefaultRayTracingPipeline();

  // create shader binding table
  this->CreateShaderBindingTable();

  // create default descriptor set
  this->CreateDefaultDescriptorSet();

  // setup buffer region addresses
  this->SetupBufferRegionAddresses();

  // create default command buffers
  this->CreateDefaultCommandBuffers();
}

// void gtp::RenderBase::CreateDefaultColorStorageImage() {
//   Utilities_Renderer::CreateColorStorageImage(
//       this->pEngineCore, &this->storageImages->defaultColor_1_bit,
//       "render_base_default_color_storage_image_");
// }

void gtp::RenderBase::CreateDefaultRayTracingPipeline() {
  // image count
  uint32_t imageCount{0};

  for (int i = 0; i < this->assets.defaultTextures.size(); i++) {
    imageCount += static_cast<uint32_t>(this->assets.defaultTextures.size());
  }

  for (int i = 0; i < assets.models.size(); i++) {
    std::cout << "create pipeline model names: "
              << this->assets.models[i]->modelName;

    imageCount += static_cast<uint32_t>(assets.models[i]->textures.size());
  }

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

  // color id storage image layout binding
  VkDescriptorSetLayoutBinding colorIDStorageImageLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          2, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1,
          VK_SHADER_STAGE_RAYGEN_BIT_KHR, nullptr);

  // uniform buffer layout binding
  VkDescriptorSetLayoutBinding uniformBufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
          VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_MISS_BIT_KHR,
          nullptr);

  // g_node_buffer layout binding
  VkDescriptorSetLayoutBinding g_node_bufferLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          nullptr);

  // g_node_indices layout binding
  VkDescriptorSetLayoutBinding g_node_indicesLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_RAYGEN_BIT_KHR,
          nullptr);

  // glass texture layout binding
  VkDescriptorSetLayoutBinding glassTextureImagesLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR | VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
          nullptr);

  // cubemap texture layout binding
  VkDescriptorSetLayoutBinding cubemapTextureImagesLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
          nullptr);

  // all texture image layout binding
  VkDescriptorSetLayoutBinding allTextureImagesLayoutBinding =
      gtp::Utilities_EngCore::VkInitializers::descriptorSetLayoutBinding(
          8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount,
          VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR |
              VK_SHADER_STAGE_ANY_HIT_BIT_KHR | VK_SHADER_STAGE_MISS_BIT_KHR,
          nullptr);

  // bindings array
  std::vector<VkDescriptorSetLayoutBinding> bindings({
      accelerationStructureLayoutBinding,
      storageImageLayoutBinding,
      colorIDStorageImageLayoutBinding,
      uniformBufferLayoutBinding,
      g_node_bufferLayoutBinding,
      g_node_indicesLayoutBinding,
      glassTextureImagesLayoutBinding,
      cubemapTextureImagesLayoutBinding,
      allTextureImagesLayoutBinding,

  });

  // Unbound set
  VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlags{};
  setLayoutBindingFlags.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT;
  setLayoutBindingFlags.bindingCount = 9;
  std::vector<VkDescriptorBindingFlagsEXT> descriptorBindingFlags = {
      0, 0, 0,
      0, 0, 0,
      0, 0, VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT};

  setLayoutBindingFlags.pBindingFlags = descriptorBindingFlags.data();

  VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo{};
  descriptorSetLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  descriptorSetLayoutCreateInfo.bindingCount =
      static_cast<uint32_t>(bindings.size());
  descriptorSetLayoutCreateInfo.pBindings = bindings.data();
  descriptorSetLayoutCreateInfo.pNext = &setLayoutBindingFlags;

  // create descriptor set layout
  pEngineCore->AddObject(
      [this, &descriptorSetLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorSetLayout(
            &descriptorSetLayoutCreateInfo, nullptr,
            &pipelineData.descriptorSetLayout);
      },
      "mainRenderer_DescriptorSetLayout");

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
  pipelineLayoutCreateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutCreateInfo.setLayoutCount = 1;
  pipelineLayoutCreateInfo.pSetLayouts = &pipelineData.descriptorSetLayout;

  // create pipeline layout
  pEngineCore->AddObject(
      [this, &pipelineLayoutCreateInfo]() {
        return pEngineCore->objCreate.VKCreatePipelineLayout(
            &pipelineLayoutCreateInfo, nullptr, &pipelineData.pipelineLayout);
      },
      "mainRenderer_RayTracingPipelineLayout");

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
    shaderStages.push_back(this->tools.shader->loadShader(
        gtp::Utilities_EngCore::BuildPath(
            "shaders/compiled/main_renderer_raygen.rgen.spv"),
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
    this->tools.shaderBindingTableData.shaderGroups.push_back(shaderGroup);
  }

  // Miss group
  {
    shaderStages.push_back(this->tools.shader->loadShader(
        projDirectory.string() +
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
    this->tools.shaderBindingTableData.shaderGroups.push_back(shaderGroup);
    // Second shader for glTFShadows
    shaderStages.push_back(this->tools.shader->loadShader(
        projDirectory.string() +
            "/shaders/compiled/main_renderer_shadow.rmiss.spv",
        VK_SHADER_STAGE_MISS_BIT_KHR, "main_renderer_miss_shadow"));
    shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
    this->tools.shaderBindingTableData.shaderGroups.push_back(shaderGroup);
  }

  // Closest hit group for doing texture lookups
  {
    shaderStages.push_back(this->tools.shader->loadShader(
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
    shaderStages.push_back(this->tools.shader->loadShader(
        projDirectory.string() +
            "/shaders/compiled/main_renderer_anyhit.rahit.spv",
        VK_SHADER_STAGE_ANY_HIT_BIT_KHR, "MainRenderer_anyhit"));
    shaderGroup.anyHitShader = static_cast<uint32_t>(shaderStages.size()) - 1;
    this->tools.shaderBindingTableData.shaderGroups.push_back(shaderGroup);
  }

  //	Create the ray tracing pipeline
  VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
  rayTracingPipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  rayTracingPipelineCreateInfo.stageCount =
      static_cast<uint32_t>(shaderStages.size());
  rayTracingPipelineCreateInfo.pStages = shaderStages.data();
  rayTracingPipelineCreateInfo.groupCount = static_cast<uint32_t>(
      this->tools.shaderBindingTableData.shaderGroups.size());
  rayTracingPipelineCreateInfo.pGroups =
      this->tools.shaderBindingTableData.shaderGroups.data();
  rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
  rayTracingPipelineCreateInfo.layout = pipelineData.pipelineLayout;

  // create raytracing pipeline
  pEngineCore->AddObject(
      [this, &rayTracingPipelineCreateInfo]() {
        return pEngineCore->objCreate.VKCreateRaytracingPipeline(
            &rayTracingPipelineCreateInfo, nullptr, &pipelineData.pipeline);
      },
      "mainRenderer_RaytracingPipeline");
}

void gtp::RenderBase::CreateShaderBindingTable() {
  // handle size
  const uint32_t handleSize =
      pEngineCore->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize;

  // aligned handle size
  const uint32_t handleSizeAligned = gtp::Utilities_EngCore::alignedSize(
      pEngineCore->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
      pEngineCore->deviceProperties.rayTracingPipelineKHR
          .shaderGroupHandleAlignment);

  // group count
  const uint32_t groupCount = static_cast<uint32_t>(
      this->tools.shaderBindingTableData.shaderGroups.size());

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
    this->tools.shaderBindingTableData.raygenShaderBindingTable.bufferData
        .bufferName = "RaygenShaderBindingTable";
    this->tools.shaderBindingTableData.raygenShaderBindingTable.bufferData
        .bufferMemoryName = "RaygenShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(
            bufferUsageFlags, memoryUsageFlags,
            &this->tools.shaderBindingTableData.raygenShaderBindingTable,
            handleSize, nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create raygenShaderBindingTable");
    }

    // miss
    this->tools.shaderBindingTableData.missShaderBindingTable.bufferData
        .bufferName = "MissShaderBindingTable";
    this->tools.shaderBindingTableData.missShaderBindingTable.bufferData
        .bufferMemoryName = "MissShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(
            bufferUsageFlags, memoryUsageFlags,
            &this->tools.shaderBindingTableData.missShaderBindingTable,
            handleSize * 2, nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create missShaderBindingTable");
    }

    // hit
    this->tools.shaderBindingTableData.hitShaderBindingTable.bufferData
        .bufferName = "HitShaderBindingTable";
    this->tools.shaderBindingTableData.hitShaderBindingTable.bufferData
        .bufferMemoryName = "HitShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(
            bufferUsageFlags, memoryUsageFlags,
            &this->tools.shaderBindingTableData.hitShaderBindingTable,
            handleSize * 2, nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create hitShaderBindingTable");
    }

    // copy buffers
    this->tools.shaderBindingTableData.raygenShaderBindingTable.map(
        pEngineCore->devices.logical);
    memcpy(this->tools.shaderBindingTableData.raygenShaderBindingTable
               .bufferData.mapped,
           shaderHandleStorage.data(), handleSize);
    this->tools.shaderBindingTableData.raygenShaderBindingTable.unmap(
        pEngineCore->devices.logical);

    this->tools.shaderBindingTableData.missShaderBindingTable.map(
        pEngineCore->devices.logical);
    memcpy(this->tools.shaderBindingTableData.missShaderBindingTable.bufferData
               .mapped,
           shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
    this->tools.shaderBindingTableData.missShaderBindingTable.unmap(
        pEngineCore->devices.logical);

    this->tools.shaderBindingTableData.hitShaderBindingTable.map(
        pEngineCore->devices.logical);
    memcpy(this->tools.shaderBindingTableData.hitShaderBindingTable.bufferData
               .mapped,
           shaderHandleStorage.data() + handleSizeAligned * 3, handleSize * 2);
    this->tools.shaderBindingTableData.hitShaderBindingTable.unmap(
        pEngineCore->devices.logical);
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

void gtp::RenderBase::CreateDefaultUniformBuffer() {

  buffers.ubo.bufferData.bufferName = "UBOBuffer";
  buffers.ubo.bufferData.bufferMemoryName = "UBOBufferMemory";

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.ubo,
                                sizeof(Utilities_Renderer::UniformData),
                                &uniformData) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create  uniform buffer!");
  }

  this->UpdateDefaultUniformBuffer(0.0f, glm::vec4(0.0f));
}

void gtp::RenderBase::CreateDefaultDescriptorSet() {
  // image count
  uint32_t imageCount{0};

  for (int i = 0; i < this->assets.defaultTextures.size(); i++) {
    imageCount += static_cast<uint32_t>(this->assets.defaultTextures.size());
  }

  for (int i = 0; i < assets.models.size(); i++) {
    imageCount += static_cast<uint32_t>(assets.models[i]->textures.size());
  }

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
  pEngineCore->AddObject(
      [this, &descriptorPoolCreateInfo]() {
        return pEngineCore->objCreate.VKCreateDescriptorPool(
            &descriptorPoolCreateInfo, nullptr,
            &this->pipelineData.descriptorPool);
      },
      "mainRenderer_DescriptorPool");

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
  pEngineCore->AddObject(
      [this, &descriptorSetAllocateInfo]() {
        return pEngineCore->objCreate.VKAllocateDescriptorSet(
            &descriptorSetAllocateInfo, nullptr,
            &this->pipelineData.descriptorSet);
      },
      "mainRenderer_DescriptorSet");

  VkAccelerationStructureKHR topLevelAccelerationStructureKHR =
      this->GetTopLevelAccelerationStructureKHR();

  VkWriteDescriptorSetAccelerationStructureKHR
      descriptorAccelerationStructureInfo{};
  descriptorAccelerationStructureInfo.sType =
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
  descriptorAccelerationStructureInfo.pAccelerationStructures =
      &topLevelAccelerationStructureKHR;

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
      VK_NULL_HANDLE, storageImages->defaultColor_1_bit->view,
      VK_IMAGE_LAYOUT_GENERAL};

  // storage/result image write
  VkWriteDescriptorSet storageImageWrite{};
  storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageImageWrite.dstSet = pipelineData.descriptorSet;
  storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  storageImageWrite.dstBinding = 1;
  storageImageWrite.pImageInfo = &storageImageDescriptor;
  storageImageWrite.descriptorCount = 1;

  VkDescriptorImageInfo colorIDStorageImageDescriptor{
      VK_NULL_HANDLE, this->tools.objectMouseSelect->GetIDImage().view,
      VK_IMAGE_LAYOUT_GENERAL};

  // storage/result image write
  VkWriteDescriptorSet colorIDStorageImageWrite{};
  colorIDStorageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  colorIDStorageImageWrite.dstSet = pipelineData.descriptorSet;
  colorIDStorageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  colorIDStorageImageWrite.dstBinding = 2;
  colorIDStorageImageWrite.pImageInfo = &colorIDStorageImageDescriptor;
  colorIDStorageImageWrite.descriptorCount = 1;

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
  uniformBufferWrite.dstBinding = 3;
  uniformBufferWrite.pBufferInfo = &uboDescriptor;
  uniformBufferWrite.descriptorCount = 1;

  // geometry_nodes_buffer
  VkDescriptorBufferInfo geometry_nodes_BufferDescriptor =
      this->GetGeometryNodesBufferDescriptor();

  // VkDescriptorBufferInfo geometry_nodes_BufferDescriptor{
  //     this->buffers.geometry_nodes_buffer.bufferData.buffer, 0,
  //     this->buffers.geometry_nodes_buffer.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet geometry_nodes_bufferWrite{};
  geometry_nodes_bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  geometry_nodes_bufferWrite.dstSet = pipelineData.descriptorSet;
  geometry_nodes_bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  geometry_nodes_bufferWrite.dstBinding = 4;
  geometry_nodes_bufferWrite.pBufferInfo = &geometry_nodes_BufferDescriptor;
  geometry_nodes_bufferWrite.descriptorCount = 1;

  // geometry_nodes_indices
  VkDescriptorBufferInfo geometry_nodes_indicesDescriptor =
      this->GetGeometryNodesIndexBufferDescriptor();

  // VkDescriptorBufferInfo geometry_nodes_indicesDescriptor{
  //     this->buffers.geometry_nodes_indices.bufferData.buffer, 0,
  //     this->buffers.geometry_nodes_indices.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet geometry_nodes_indicesWrite{};
  geometry_nodes_indicesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  geometry_nodes_indicesWrite.dstSet = pipelineData.descriptorSet;
  geometry_nodes_indicesWrite.descriptorType =
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  geometry_nodes_indicesWrite.dstBinding = 5;
  geometry_nodes_indicesWrite.pBufferInfo = &geometry_nodes_indicesDescriptor;
  geometry_nodes_indicesWrite.descriptorCount = 1;

  VkDescriptorImageInfo glassTextureDescriptor{
      this->assets.coloredGlassTexture.sampler,
      this->assets.coloredGlassTexture.view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  // glass texture image write
  VkWriteDescriptorSet glassTextureWrite{};
  glassTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  glassTextureWrite.dstSet = pipelineData.descriptorSet;
  glassTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glassTextureWrite.dstBinding = 6;
  glassTextureWrite.pImageInfo = &glassTextureDescriptor;
  glassTextureWrite.descriptorCount = 1;

  VkDescriptorImageInfo cubemapTextureDescriptor{
      this->assets.cubemap.sampler, this->assets.cubemap.view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  // cubemap texture image write
  VkWriteDescriptorSet cubemapTextureWrite{};
  cubemapTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  cubemapTextureWrite.dstSet = pipelineData.descriptorSet;
  cubemapTextureWrite.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  cubemapTextureWrite.dstBinding = 7;
  cubemapTextureWrite.pImageInfo = &cubemapTextureDescriptor;
  cubemapTextureWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 0: Top level acceleration structure
      accelerationStructureWrite,
      // Binding 1: Ray tracing result image
      storageImageWrite,
      // Binding 1: Ray tracing result image
      colorIDStorageImageWrite,
      // Binding 2: Uniform data
      uniformBufferWrite,
      // Binding 3: geometry_nodes_buffer write
      geometry_nodes_bufferWrite,
      // Binding 4: geometry_nodes_indices write
      geometry_nodes_indicesWrite,
      // Binding 5: glass texture image write
      glassTextureWrite,
      // Binding 6: cubemap texture image write
      cubemapTextureWrite};

  // Image descriptors for the image array
  std::vector<VkDescriptorImageInfo> textureDescriptors{};

  for (int i = 0; i < this->assets.defaultTextures.size(); i++) {
    VkDescriptorImageInfo descriptor{};
    descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor.sampler = this->assets.defaultTextures[i].sampler;
    descriptor.imageView = this->assets.defaultTextures[i].view;
    textureDescriptors.push_back(descriptor);
  }

  if (!assets.models.empty()) {
    for (int i = 0; i < assets.models.size(); i++) {
      for (int j = 0; j < assets.models[i]->textures.size(); j++) {
        VkDescriptorImageInfo descriptor{};
        descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptor.sampler = assets.models[i]->textures[j].sampler;
        descriptor.imageView = assets.models[i]->textures[j].view;
        textureDescriptors.push_back(descriptor);
      }
    }
  }

  VkWriteDescriptorSet writeDescriptorImgArray{};
  writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorImgArray.dstBinding = 8;
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

void gtp::RenderBase::SetupBufferRegionAddresses() {
  // setup buffer regions pointing to shaders in shader binding table
  const uint32_t handleSizeAligned = gtp::Utilities_EngCore::alignedSize(
      pEngineCore->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
      pEngineCore->deviceProperties.rayTracingPipelineKHR
          .shaderGroupHandleAlignment);

  // VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
  this->tools.shaderBindingTableData.raygenStridedDeviceAddressRegion
      .deviceAddress = pEngineCore->GetBufferDeviceAddress(
      this->tools.shaderBindingTableData.raygenShaderBindingTable.bufferData
          .buffer);
  this->tools.shaderBindingTableData.raygenStridedDeviceAddressRegion.stride =
      handleSizeAligned;
  this->tools.shaderBindingTableData.raygenStridedDeviceAddressRegion.size =
      handleSizeAligned;

  // VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
  this->tools.shaderBindingTableData.missStridedDeviceAddressRegion
      .deviceAddress = pEngineCore->GetBufferDeviceAddress(
      this->tools.shaderBindingTableData.missShaderBindingTable.bufferData
          .buffer);
  this->tools.shaderBindingTableData.missStridedDeviceAddressRegion.stride =
      handleSizeAligned;
  this->tools.shaderBindingTableData.missStridedDeviceAddressRegion.size =
      handleSizeAligned * 2;

  // VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
  this->tools.shaderBindingTableData.hitStridedDeviceAddressRegion
      .deviceAddress = pEngineCore->GetBufferDeviceAddress(
      this->tools.shaderBindingTableData.hitShaderBindingTable.bufferData
          .buffer);
  this->tools.shaderBindingTableData.hitStridedDeviceAddressRegion.stride =
      handleSizeAligned;
  this->tools.shaderBindingTableData.hitStridedDeviceAddressRegion.size =
      handleSizeAligned * 2;
}

void gtp::RenderBase::CreateDefaultCommandBuffers() {
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
        &this->tools.shaderBindingTableData.raygenStridedDeviceAddressRegion,
        &this->tools.shaderBindingTableData.missStridedDeviceAddressRegion,
        &this->tools.shaderBindingTableData.hitStridedDeviceAddressRegion,
        &emptyShaderSbtEntry,
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
        pEngineCore->commandBuffers.graphics[i],
        storageImages->defaultColor_1_bit->image, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {pEngineCore->swapchainData.swapchainExtent2D.width,
                         pEngineCore->swapchainData.swapchainExtent2D.height,
                         1};

    vkCmdCopyImage(pEngineCore->commandBuffers.graphics[i],
                   storageImages->defaultColor_1_bit->image,
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
        pEngineCore->commandBuffers.graphics[i],
        storageImages->defaultColor_1_bit->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange);

    if (vkEndCommandBuffer(pEngineCore->commandBuffers.graphics[i]) !=
        VK_SUCCESS) {
      throw std::invalid_argument("failed to end recording command buffer");
    }
  }
}

void gtp::RenderBase::RebuildCommandBuffers(int frame, bool showObjectColorID) {
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
      &this->tools.shaderBindingTableData.raygenStridedDeviceAddressRegion,
      &this->tools.shaderBindingTableData.missStridedDeviceAddressRegion,
      &this->tools.shaderBindingTableData.hitStridedDeviceAddressRegion,
      &emptySbtEntry, pEngineCore->swapchainData.swapchainExtent2D.width,
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

  if (!showObjectColorID) {
    gtp::Utilities_EngCore::setImageLayout(
        pEngineCore->commandBuffers.graphics[frame],
        storageImages->defaultColor_1_bit->image, VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

    VkImageCopy copyRegion{};
    copyRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.srcOffset = {0, 0, 0};
    copyRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copyRegion.dstOffset = {0, 0, 0};
    copyRegion.extent = {pEngineCore->swapchainData.swapchainExtent2D.width,
                         pEngineCore->swapchainData.swapchainExtent2D.height,
                         1};

    vkCmdCopyImage(pEngineCore->commandBuffers.graphics[frame],
                   storageImages->defaultColor_1_bit->image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
        pEngineCore->commandBuffers.graphics[frame],
        storageImages->defaultColor_1_bit->image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange);
  }

  else {
    gtp::Utilities_EngCore::setImageLayout(
        pEngineCore->commandBuffers.graphics[frame],
        this->tools.objectMouseSelect->GetIDImage().image,
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

    vkCmdCopyImage(pEngineCore->commandBuffers.graphics[frame],
                   this->tools.objectMouseSelect->GetIDImage().image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
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
        pEngineCore->commandBuffers.graphics[frame],
        this->tools.objectMouseSelect->GetIDImage().image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange);
  }
}

void gtp::RenderBase::SetupModelDataTransforms(
    Utilities_UI::TransformMatrices *pTransformMatrices) {

  // transform matrices
  Utilities_UI::TransformMatrices transformMatrices{};

  // transform values
  // related to UI slider floats that will be used to manipulate transform
  // matrices
  Utilities_UI::TransformValues transformValues{};

  // assign transform matrices if passed in on load
  // pre transforms
  if (pTransformMatrices != nullptr) {
    transformMatrices.rotate = pTransformMatrices->rotate;

    transformMatrices.translate = pTransformMatrices->translate;

    transformMatrices.scale = pTransformMatrices->scale;
  }
  // rotate transform values
  //  Extract the upper 3x3 part of the matrix
  glm::mat3 rotationMatrix = glm::mat3(transformMatrices.rotate);

  // Normalize the columns to remove scaling
  rotationMatrix[0] = glm::normalize(rotationMatrix[0]);
  rotationMatrix[1] = glm::normalize(rotationMatrix[1]);
  rotationMatrix[2] = glm::normalize(rotationMatrix[2]);

  // Convert the 3x3 rotation matrix to a quaternion
  glm::quat rotationQuaternion = glm::quat_cast(rotationMatrix);

  // Convert the quaternion to a vec4 (xyzw)
  glm::vec4 rotationVec4 =
      glm::vec4(rotationQuaternion.x, rotationQuaternion.y,
                rotationQuaternion.z, rotationQuaternion.w);

  // Assign it to transformValues.rotate
  transformValues.rotate = rotationVec4;

  // translate transform values
  transformValues.translate = glm::vec4(transformMatrices.translate[3]);

  // scale transform values
  transformValues.scale =
      glm::vec4(transformMatrices.scale * glm::vec4(1.0f)).x;

  // add transform values/matrices to lists
  this->assets.modelData.transformMatrices.push_back(transformMatrices);
  this->assets.modelData.transformValues.push_back(transformValues);
}

void gtp::RenderBase::LoadGltfCompute(gtp::Model *pModel) {

  ComputeVertex *computeVtx = nullptr;

  if (!pModel->animations.empty()) {
    auto uniqueComputeVertex =
        std::make_unique<ComputeVertex>(this->pEngineCore, pModel);
    computeVtx = uniqueComputeVertex.release();
  }

  this->assets.gltfCompute.push_back(computeVtx);
}

void gtp::RenderBase::UpdateDefaultRayTracingPipeline() {
  // -- pipeline and layout
  this->pipelineData.Destroy(this->pEngineCore);

  this->CreateDefaultRayTracingPipeline();
}

void gtp::RenderBase::UpdateDefaultDescriptorSet() {
  // image count
  uint32_t imageCount{0};

  for (int i = 0; i < this->assets.defaultTextures.size(); i++) {
    imageCount += static_cast<uint32_t>(this->assets.defaultTextures.size());
  }

  for (int i = 0; i < assets.models.size(); i++) {
    std::cout << "update descriptor set model names: "
              << this->assets.models[i]->modelName;
    imageCount += static_cast<uint32_t>(assets.models[i]->textures.size());
  }

  VkAccelerationStructureKHR topLevelAccelerationStructureKHR =
      this->GetTopLevelAccelerationStructureKHR();

  VkWriteDescriptorSetAccelerationStructureKHR
      descriptorAccelerationStructureInfo{};
  descriptorAccelerationStructureInfo.sType =
      VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
  descriptorAccelerationStructureInfo.accelerationStructureCount = 1;
  descriptorAccelerationStructureInfo.pAccelerationStructures =
      &topLevelAccelerationStructureKHR;
  // &this->TLAS.accelerationStructureKHR;

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
      VK_NULL_HANDLE, storageImages->defaultColor_1_bit->view,
      VK_IMAGE_LAYOUT_GENERAL};

  // storage/result image write
  VkWriteDescriptorSet storageImageWrite{};
  storageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  storageImageWrite.dstSet = pipelineData.descriptorSet;
  storageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  storageImageWrite.dstBinding = 1;
  storageImageWrite.pImageInfo = &storageImageDescriptor;
  storageImageWrite.descriptorCount = 1;

  VkDescriptorImageInfo colorIDStorageImageDescriptor{
      VK_NULL_HANDLE, tools.objectMouseSelect->GetIDImage().view,
      VK_IMAGE_LAYOUT_GENERAL};

  // storage/result image write
  VkWriteDescriptorSet colorIDStorageImageWrite{};
  colorIDStorageImageWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  colorIDStorageImageWrite.dstSet = pipelineData.descriptorSet;
  colorIDStorageImageWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  colorIDStorageImageWrite.dstBinding = 2;
  colorIDStorageImageWrite.pImageInfo = &colorIDStorageImageDescriptor;
  colorIDStorageImageWrite.descriptorCount = 1;

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
  uniformBufferWrite.dstBinding = 3;
  uniformBufferWrite.pBufferInfo = &uboDescriptor;
  uniformBufferWrite.descriptorCount = 1;

  // geometry_nodes_buffer
  // VkDescriptorBufferInfo geometry_nodes_BufferDescriptor{
  //    this->buffers.geometry_nodes_buffer.bufferData.buffer, 0,
  //    this->buffers.geometry_nodes_buffer.bufferData.size};

  // geometry_nodes_buffer
  VkDescriptorBufferInfo geometry_nodes_BufferDescriptor =
      this->GetGeometryNodesBufferDescriptor();

  // geometry descriptor write info
  VkWriteDescriptorSet geometry_nodes_bufferWrite{};
  geometry_nodes_bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  geometry_nodes_bufferWrite.dstSet = pipelineData.descriptorSet;
  geometry_nodes_bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  geometry_nodes_bufferWrite.dstBinding = 4;
  geometry_nodes_bufferWrite.pBufferInfo = &geometry_nodes_BufferDescriptor;
  geometry_nodes_bufferWrite.descriptorCount = 1;

  // geometry_nodes_indices
  VkDescriptorBufferInfo geometry_nodes_indicesDescriptor =
      this->GetGeometryNodesIndexBufferDescriptor();
  // VkDescriptorBufferInfo geometry_nodes_indicesDescriptor{
  //     this->buffers.geometry_nodes_indices.bufferData.buffer, 0,
  //     this->buffers.geometry_nodes_indices.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet geometry_nodes_indicesWrite{};
  geometry_nodes_indicesWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  geometry_nodes_indicesWrite.dstSet = pipelineData.descriptorSet;
  geometry_nodes_indicesWrite.descriptorType =
      VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  geometry_nodes_indicesWrite.dstBinding = 5;
  geometry_nodes_indicesWrite.pBufferInfo = &geometry_nodes_indicesDescriptor;
  geometry_nodes_indicesWrite.descriptorCount = 1;

  VkDescriptorImageInfo glassTextureDescriptor{
      this->assets.coloredGlassTexture.sampler,
      this->assets.coloredGlassTexture.view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  // glass texture image write
  VkWriteDescriptorSet glassTextureWrite{};
  glassTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  glassTextureWrite.dstSet = pipelineData.descriptorSet;
  glassTextureWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  glassTextureWrite.dstBinding = 6;
  glassTextureWrite.pImageInfo = &glassTextureDescriptor;
  glassTextureWrite.descriptorCount = 1;

  VkDescriptorImageInfo cubemapTextureDescriptor{
      this->assets.cubemap.sampler, this->assets.cubemap.view,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

  // cubemap texture image write
  VkWriteDescriptorSet cubemapTextureWrite{};
  cubemapTextureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  cubemapTextureWrite.dstSet = pipelineData.descriptorSet;
  cubemapTextureWrite.descriptorType =
      VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  cubemapTextureWrite.dstBinding = 7;
  cubemapTextureWrite.pImageInfo = &cubemapTextureDescriptor;
  cubemapTextureWrite.descriptorCount = 1;

  std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
      // Binding 0: Top level acceleration structure
      accelerationStructureWrite,
      // Binding 1: Ray tracing result image
      storageImageWrite,
      // Binding 1: Ray tracing result image
      colorIDStorageImageWrite,
      // Binding 2: Uniform data
      uniformBufferWrite,
      // Binding 3: geometry_nodes_buffer write
      geometry_nodes_bufferWrite,
      // Binding 4: geometry_nodes_indices write
      geometry_nodes_indicesWrite,
      // Binding 5: glass texture image write
      glassTextureWrite,
      // Binding 6: cubemap texture image write
      cubemapTextureWrite};

  // Image descriptors for the image array
  std::vector<VkDescriptorImageInfo> textureDescriptors{};

  for (int i = 0; i < this->assets.defaultTextures.size(); i++) {
    VkDescriptorImageInfo descriptor{};
    descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptor.sampler = this->assets.defaultTextures[i].sampler;
    descriptor.imageView = this->assets.defaultTextures[i].view;
    textureDescriptors.push_back(descriptor);
  }

  if (!assets.models.empty()) {
    for (int i = 0; i < assets.models.size(); i++) {
      for (int j = 0; j < assets.models[i]->textures.size(); j++) {
        VkDescriptorImageInfo descriptor{};
        descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        descriptor.sampler = assets.models[i]->textures[j].sampler;
        descriptor.imageView = assets.models[i]->textures[j].view;
        textureDescriptors.push_back(descriptor);
      }
    }
  }

  VkWriteDescriptorSet writeDescriptorImgArray{};
  writeDescriptorImgArray.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  writeDescriptorImgArray.dstBinding = 8;
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

/*  initialize constructor  */
gtp::RenderBase::RenderBase(EngineCore *engineCorePtr)
    : gtp::AccelerationStructures(engineCorePtr) {
  // call initialize function
  this->InitializeRenderBase(engineCorePtr);
}

void gtp::RenderBase::UpdateDefaultUniformBuffer(float deltaTime,
                                                 glm::vec4 lightPosition) {
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

  uniformData.lightPos = lightPosition;

  uniformData.viewPos = glm::vec4(this->pEngineCore->camera->Position, 1.0f);

  memcpy(buffers.ubo.bufferData.mapped, &uniformData,
         sizeof(Utilities_Renderer::UniformData));
}

/*  destroy the base class resources  */
void gtp::RenderBase::DestroyRenderBase() {

  // descriptor pool && associated descriptor sets
  // pipeline data
  this->pipelineData.Destroy(this->pEngineCore);
  // -- shader binding tables
  this->tools.shaderBindingTableData.Destroy(this->pEngineCore);
  // -- uniform buffer
  this->buffers.ubo.destroy(this->pEngineCore->GetLogicalDevice());
  // -- default color storage image
  this->storageImages->defaultColor_1_bit->Destroy(this->pEngineCore);
  // -- acceleration structures
  this->DestroyAccelerationStructures();
  // -- assets
  this->assets.DestroyDefaultAssets(this->pEngineCore);
  // -- shaders
  this->tools.shader->DestroyShader();
  // -- object mouse select
  this->tools.objectMouseSelect->DestroyObjectMouseSelect();
}

Utilities_UI::ModelData *gtp::RenderBase::GetModelData() {
  return &this->assets.modelData;
}

void gtp::RenderBase::SetModelData(Utilities_UI::ModelData *pModelData) {
  this->assets.modelData = *pModelData;
  this->assets.modelData.isUpdated = false;
}

void gtp::RenderBase::RetrieveObjectID(int posX, int posY) {
  this->tools.objectMouseSelect->RetrieveObjectID(posX, posY);
}

void gtp::RenderBase::LoadModel(
    std::string filename, uint32_t fileLoadingFlags, uint32_t modelLoadingFlags,
    Utilities_UI::TransformMatrices *pTransformMatrices) {

  // model instance pointer to initialize and add to list
  gtp::Model *tempModel = nullptr;
  auto uniqueModelPtr = std::make_unique<gtp::Model>();
  tempModel = uniqueModelPtr.release();

  // model index
  auto modelIdx = static_cast<int>(this->assets.models.size());

  // -- Load From File ---- gtp::Model function
  tempModel->loadFromFile(filename, pEngineCore, pEngineCore->queue.graphics,
                          fileLoadingFlags);

  // set semi transparent flag
  const bool isSemiTransparent =
      modelLoadingFlags ==
              Utilities_Renderer::ModelLoadingFlags::SemiTransparent
          ? true
          : false;

  if (isSemiTransparent || this->assets.modelData.loadModelSemiTransparent) {
    tempModel->semiTransparentFlag = static_cast<int>(true);
  }

  // Set "isAnimated" flag and add to list
  // referenced by blas/tlas/compute vertex
  const bool isAnimated = tempModel->animations.empty() ? false : true;

  this->assets.modelData.animatedModelIndex.push_back(
      static_cast<int>(isAnimated));

  // add animated toggle to list
  this->assets.modelData.isAnimated.push_back(false);

  // push back update blas
  this->assets.modelData.updateBLAS.push_back(false);

  // add active animation and animation name to their lists.
  // assign 0 and "none" if model is not animated
  std::vector<std::string> tempNames;
  std::vector<int> tempAnimationIndex;

  if (tempModel->animations.empty()) {
    tempAnimationIndex.push_back(0);
    this->assets.modelData.activeAnimation.push_back(tempAnimationIndex);
    tempNames.push_back("none");
    this->assets.modelData.animationNames.push_back(tempNames);
  }

  else {
    tempAnimationIndex.push_back(0);
    this->assets.modelData.activeAnimation.push_back(tempAnimationIndex);
    for (int i = 0; i < tempModel->animations.size(); i++) {
      tempNames.push_back(tempModel->animations[i].name);
      std::cout << tempModel->animations[i].name << std::endl;
    }
    this->assets.modelData.animationNames.push_back(tempNames);
  }

  // add model name to model data's model name list
  this->assets.modelData.modelName.push_back(tempModel->modelName);

  // add model to model list
  this->assets.models.push_back(tempModel);

  // transform values
  // related to UI slider floats that will be used to manipulate transform
  this->SetupModelDataTransforms(pTransformMatrices);

  // load gltf -- internally conditional if model contains animations else ==
  // nullptr
  LoadGltfCompute(tempModel);

  // create bottom level acceleration structure for model
  // CreateBLAS(tempModel);

  // acceleration structures class refactor
  this->BuildBottomLevelAccelerationStructure(tempModel);

  // not a particle
  this->assets.particle.push_back(nullptr);
}

void gtp::RenderBase::HandleLoadModel(gtp::FileLoadingFlags loadingFlags) {
  // call main renderer load model function
  this->LoadModel(this->assets.modelData.loadModelFilepath, loadingFlags);

  // update main renderer geometry nodes buffer
  // this->UpdateGeometryNodesBuffer(
  //    this->assets.models[this->assets.models.size() - 1]);

  // acceleration structure class rebuild geometry buffer
  this->RebuildGeometryBuffer(
      this->assets.models[this->assets.models.size() - 1], this->assets.models,
      this->assets.modelData, this->assets.particle);

  this->UpdateDefaultRayTracingPipeline();
  this->CreateDefaultDescriptorSet();

  // output loaded model name
  std::cout << "loaded model name from engine: "
            << this->assets.models[this->assets.models.size() - 1]->modelName
            << std::endl;

  // set main renderer model data load model flag to false
  this->assets.modelData.loadModel = false;

  // set main renderer model data semi transparency load flag to false
  this->assets.modelData.loadModelSemiTransparent = false;

  // set main renderer model data load model file path to " "
  this->assets.modelData.loadModelFilepath = " ";
}

void gtp::RenderBase::HandleResize() {
  // Delete allocated resources
  this->storageImages->defaultColor_1_bit->Destroy(this->pEngineCore);

  // recreate object id resources
  this->tools.objectMouseSelect->HandleResize();

  // recreate renderer storage image
  this->storageImages->CreateStorageImages();

  // Update descriptor
  this->UpdateDefaultDescriptorSet();
}

std::vector<VkCommandBuffer> gtp::RenderBase::RecordCompute(int frame) {
  std::vector<VkCommandBuffer> computeBuffers;

  int modelIdx = 0;

  for (auto &vertexCompute : this->assets.gltfCompute) {
    if (vertexCompute != nullptr) {
      vertexCompute->UpdateTransformsBuffer(
          &this->assets.modelData.transformMatrices[modelIdx]);
      computeBuffers.push_back(vertexCompute->RecordComputeCommands(frame));
    }
    ++modelIdx;
  }

  for (auto &particleCompute : this->assets.particle) {
    if (particleCompute != nullptr) {
      computeBuffers.push_back(particleCompute->RecordComputeCommands(frame));
    }
  }
  return computeBuffers;
}

void gtp::RenderBase::UpdateAnimations(float deltaTime) {
  for (size_t i = 0; i < this->assets.models.size(); ++i) {
    // check animated index to check against model list for animated models
    if (this->assets.modelData.animatedModelIndex[i] == 1) {
      // get current model animation
      int activeAnimation = this->assets.modelData.activeAnimation[i][0];
      // verify current model is being animated
      if (this->assets.modelData.isAnimated[i]) {
        this->assets.models[i]->updateAnimation(
            activeAnimation, deltaTime,
            &this->assets.gltfCompute[i]->jointBuffer);
      }
    }
  }
}

void gtp::RenderBase::DeleteModel() {
  // model idx
  int modelIdx = this->assets.modelData.modelIndex;

  std::cout << " deleting model" << std::endl;

  std::cout << "\tname: " << this->assets.models[modelIdx]->modelName
            << std::endl;
  std::cout << "\tindex: " << this->assets.modelData.modelIndex << std::endl;

  // wait for device idle
  vkDeviceWaitIdle(this->pEngineCore->devices.logical);

  // clear blas instances 'buffer'
  this->ClearBottomLevelAccelerationStructureInstances();
  // this->tlasData.bottomLevelAccelerationStructureInstances.clear();

  // destroy particle class if one is associated
  if (this->assets.models[this->assets.modelData.modelIndex]->isParticle) {
    this->assets.particle[this->assets.modelData.modelIndex]->DestroyParticle();
  }

  // erase model index associated particle
  this->assets.particle.erase(this->assets.particle.begin() +
                              this->assets.modelData.modelIndex);

  // destroy model
  this->assets.models[this->assets.modelData.modelIndex]->destroy(
      this->pEngineCore->devices.logical);

  // not sure prob delete
  if (this->assets.modelData.modelIndex < this->assets.models.size()) {
    this->assets.models.erase(this->assets.models.begin() +
                              this->assets.modelData.modelIndex);
  }

  // update bottom level acceleration structures
  // for (int i = 0; i < this->bottomLevelAccelerationStructures.size(); i++) {
  //  // accel. structure
  //  pEngineCore->coreExtensions->vkDestroyAccelerationStructureKHR(
  //      pEngineCore->devices.logical,
  //      this->bottomLevelAccelerationStructures[i]
  //          ->accelerationStructure.accelerationStructureKHR,
  //      nullptr);

  //  // scratch buffer
  //  this->bottomLevelAccelerationStructures[i]
  //      ->accelerationStructure.scratchBuffer.destroy(
  //          this->pEngineCore->devices.logical);

  //  // accel structure buffer and memory
  //  vkDestroyBuffer(pEngineCore->devices.logical,
  //                  this->bottomLevelAccelerationStructures[i]
  //                      ->accelerationStructure.buffer,
  //                  nullptr);
  //  vkFreeMemory(pEngineCore->devices.logical,
  //               this->bottomLevelAccelerationStructures[i]
  //                   ->accelerationStructure.memory,
  //               nullptr);
  //}

  this->HandleModelDeleteBottomLevel(
      static_cast<int>(this->assets.defaultTextures.size()),
      this->assets.models);

  // update texture offset
  // this->assets.textureOffset =
  //    static_cast<uint32_t>(this->assets.defaultTextures.size());

  //// create vector of blas pointers to reassign mainRenderer member of blas
  //// pointers with
  // std::vector<Utilities_AS::BLASData *> tempLevelAccelerationStructures;
  //
  //// clear g node buffer
  // this->geometryNodes.clear();
  //// clear g node index buffer
  // this->geometryNodesIndices.clear();
  //// clear bottom level acceleration structures "buffer"
  // this->bottomLevelAccelerationStructures.clear();
  //
  // for (int i = 0; i < this->assets.models.size(); i++) {
  //   CreateBLAS(this->assets.models[i]);
  // }
  //
  //// update g nodes buffer with updated vector of g nodes
  // this->buffers.geometry_nodes_buffer.copyTo(
  //     this->geometryNodes.data(), static_cast<uint32_t>(geometryNodes.size())
  //     *
  //                                     sizeof(Utilities_AS::GeometryNode));
  //
  //// update g node indices buffer with updated vector of g node indices
  // this->buffers.geometry_nodes_indices.copyTo(
  //     this->geometryNodesIndices.data(),
  //     static_cast<uint32_t>(geometryNodesIndices.size()) * sizeof(int));

  // erase models / modelData assignments

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.activeAnimation.size()) {
    this->assets.modelData.activeAnimation.erase(
        this->assets.modelData.activeAnimation.begin() +
        this->assets.modelData.modelIndex);
  }

  std::cout << "this->assets.modelData.animatedModelIndex.size(): "
            << this->assets.modelData.animatedModelIndex.size() << std::endl;

  std::cout << "this->assets.modelData.modelIndex: "
            << this->assets.modelData.modelIndex << std::endl;

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.animatedModelIndex.size()) {
    this->assets.modelData.animatedModelIndex.erase(
        this->assets.modelData.animatedModelIndex.begin() +
        this->assets.modelData.modelIndex);
  }

  std::cout << "post resize\nthis->assets.modelData.animatedModelIndex.size(): "
            << this->assets.modelData.animatedModelIndex.size() << std::endl;

  std::cout << "this->assets.modelData.modelIndex: "
            << this->assets.modelData.modelIndex << std::endl;

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.animationNames.size()) {
    this->assets.modelData.animationNames.erase(
        this->assets.modelData.animationNames.begin() +
        this->assets.modelData.modelIndex);
  }

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.modelName.size()) {
    this->assets.modelData.modelName.erase(
        this->assets.modelData.modelName.begin() +
        this->assets.modelData.modelIndex);
  }

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.isAnimated.size()) {
    this->assets.modelData.isAnimated.erase(
        this->assets.modelData.isAnimated.begin() +
        this->assets.modelData.modelIndex);
  }

  // if (this->assets.modelData.modelIndex <
  //     this->assets.modelData.semiTransparentFlag.size()) {
  //   this->assets.modelData.semiTransparentFlag.erase(
  //       this->assets.modelData.semiTransparentFlag.begin() +
  //       this->assets.modelData.modelIndex);
  // }

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.transformMatrices.size()) {
    this->assets.modelData.transformMatrices.erase(
        this->assets.modelData.transformMatrices.begin() +
        this->assets.modelData.modelIndex);
  }

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.transformValues.size()) {
    this->assets.modelData.transformValues.erase(
        this->assets.modelData.transformValues.begin() +
        this->assets.modelData.modelIndex);
  }

  if (this->assets.modelData.modelIndex <
      this->assets.modelData.updateBLAS.size()) {
    this->assets.modelData.updateBLAS.erase(
        this->assets.modelData.updateBLAS.begin() +
        this->assets.modelData.modelIndex);
  }

  if (this->assets.gltfCompute[this->assets.modelData.modelIndex] != nullptr) {
    this->assets.gltfCompute[this->assets.modelData.modelIndex]
        ->Destroy_ComputeVertex();
  }
  if (this->assets.modelData.modelIndex < this->assets.gltfCompute.size()) {
    this->assets.gltfCompute.erase(this->assets.gltfCompute.begin() +
                                   this->assets.modelData.modelIndex);
  }

  //// accel. structure
  // pEngineCore->coreExtensions->vkDestroyAccelerationStructureKHR(
  //     pEngineCore->devices.logical, this->TLAS.accelerationStructureKHR,
  //     nullptr);

  //// scratch buffer
  // buffers.tlas_scratch.destroy(this->pEngineCore->devices.logical);

  //// instances buffer
  // buffers.tlas_instancesBuffer.destroy(this->pEngineCore->devices.logical);

  //// accel. structure buffer and memory
  // vkDestroyBuffer(pEngineCore->devices.logical, this->TLAS.buffer, nullptr);
  // vkFreeMemory(pEngineCore->devices.logical, this->TLAS.memory, nullptr);

  //// transforms buffer
  // this->buffers.transformBuffer.destroy(this->pEngineCore->devices.logical);

  // this->assets.modelData.modelIndex = 0;

  // vkDeviceWaitIdle(this->pEngineCore->devices.logical);

  // UpdateBLAS();
  // CreateTLAS();

  //// acceleration structures class update acceleration structures
  // this->accelerationStructures.RebuildAccelerationStructures(
  //     this->assets.models.size(), this->assets.modelData);

  this->HandleModelDeleteTopLevel(this->assets.models, this->assets.modelData,
                                  this->assets.particle);

  this->UpdateDefaultDescriptorSet();

  vkDeviceWaitIdle(this->pEngineCore->devices.logical);

  this->assets.modelData.deleteModel = false;

  std::cout << "model successfully deleted!" << std::endl;
}

void gtp::RenderBase::RebuildAS() {
  this->RebuildAccelerationStructures(
      static_cast<int>(this->assets.models.size()), this->assets.modelData);
}

void gtp::RenderBase::Tools::InitializeTools(EngineCore *engineCorePtr) {

  // initialize object select
  auto uniqueObjectMousePtr =
      std::make_unique<gtp::ObjectMouseSelect>(engineCorePtr);
  this->objectMouseSelect = uniqueObjectMousePtr.release();

  // shader
  auto uniqueShaderPtr = std::make_unique<gtp::Shader>(engineCorePtr);
  this->shader = uniqueShaderPtr.release();
}

void gtp::RenderBase::Assets::LoadDefaultAssets(EngineCore *engineCorePtr) {
  // cubemap and default textures
  this->cubemap = gtp::TextureLoader(engineCorePtr);
  this->cubemap.LoadCubemap("/assets/textures/cubemaps/dsgf");

  this->defaultTextures.push_back(this->cubemap);

  // update texture offset for shader
  // this->GetTextureOffset =
  // static_cast<uint32_t>(this->defaultTextures.size());

  // -- load glass texture just because why not load a ktx
  this->coloredGlassTexture = gtp::TextureLoader(engineCorePtr);

  this->coloredGlassTexture.loadFromFile(
      "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/"
      "textures/"
      "colored_glass_rgba.ktx",
      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  const uint32_t glTFLoadingFlags =
      gtp::FileLoadingFlags::PreTransformVertices |
      gtp::FileLoadingFlags::PreMultiplyVertexColors;

  //// -- load animation
  // Utilities_UI::TransformMatrices animatedTransformMatrices{};
  //
  // animatedTransformMatrices.rotate = glm::rotate(
  //     glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
  //
  // animatedTransformMatrices.translate =
  //     glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, 0.0f, 0.0f));
  //
  // animatedTransformMatrices.scale =
  //     glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

  // this->LoadModel(
  //   "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/"
  //   "assets/models/Fox2/Fox2.gltf",
  //   gtp::FileLoadingFlags::None,
  //   Utilities_Renderer::ModelLoadingFlags::Animated,
  //   &animatedTransformMatrices);
  //
  //  -- load scene
  // this->LoadModel(
  //   "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/"
  //   "desert_scene/testScene.gltf",
  //   gtp::FileLoadingFlags::None, Utilities_Renderer::ModelLoadingFlags::None,
  //   nullptr);
  //
  //// -- load water surface
  // this->LoadModel(
  //   "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/"
  //   "desert_scene/pool_water_surface/pool_water_surface.gltf",
  //   gtp::FileLoadingFlags::None,
  //   Utilities_Renderer::ModelLoadingFlags::SemiTransparent, nullptr);

  //// -- load particle
  // this->LoadParticle("", gtp::FileLoadingFlags::None,
  //                    Utilities_Renderer::ModelLoadingFlags::None, nullptr);

  // for (int i = 0; i < this->modelData.animatedModelIndex.size(); i++) {
  //   std::cout << "\nanimated model index[" << i
  //             << "]: " << this->modelData.animatedModelIndex[i] << std::endl;
  // }
  // std::cout << "this->modelData.animatedModelIndex.size(): "
  //           << this->modelData.animatedModelIndex.size() << std::endl;
}

void gtp::RenderBase::Assets::DestroyDefaultAssets(EngineCore *engineCorePtr) {

  // particle
  for (auto &particles : this->particle) {
    if (particles != nullptr) {
      particles->DestroyParticle();
    }
  }

  // -- compute class
  for (int i = 0; i < this->gltfCompute.size(); i++) {
    if (this->gltfCompute[i] != nullptr) {
      this->gltfCompute[i]->Destroy_ComputeVertex();
    }
  }

  // -- models
  for (int i = 0; i < this->models.size(); i++) {
    if (this->models[i] != nullptr) {
      this->models[i]->destroy(engineCorePtr->GetLogicalDevice());
    }
  }

  // default textures
  for (auto &defaultTex : this->defaultTextures) {
    defaultTex.DestroyTextureLoader();
  }

  // ktx texture
  this->coloredGlassTexture.DestroyTextureLoader();
}

void gtp::RenderBase::StorageImages::CreateDefaultColorStorageImage() {
  // Create a unique_ptr to manage the StorageImage
  std::unique_ptr<Utilities_Renderer::StorageImage> uniqueColorImage =
      std::make_unique<Utilities_Renderer::StorageImage>();

  // Initialize the storage image using the raw pointer from unique_ptr
  Utilities_Renderer::CreateColorStorageImage(
      this->pEngineCore, uniqueColorImage.get(), VK_SAMPLE_COUNT_1_BIT,
      "render_base_default_color_storage_image_1_bit");

  // Assign the raw pointer to the member variable
  this->defaultColor_1_bit = uniqueColorImage.release(); // Release ownership
}

void gtp::RenderBase::StorageImages::CreateMultisampleResources() {
  Utilities_Renderer::CreateColorStorageImage(
      this->pEngineCore, &this->multisampleImage_8_bit, VK_SAMPLE_COUNT_8_BIT,
      "render_base_multisampleImage_8_bit");

  Utilities_Renderer::CreateColorStorageImage(
      this->pEngineCore, &this->multisampleImageResolve_1_bit,
      VK_SAMPLE_COUNT_1_BIT, "render_base_multisampleImageResolve_1_bit");
}

void gtp::RenderBase::StorageImages::CreateStorageImages() {
  // create default color storage image
  this->CreateDefaultColorStorageImage();
}
