#include "MainRenderer.hpp"

// Function to generate a random float between min and max
float randomFloat(float min, float max) {
  static std::default_random_engine generator;
  std::uniform_real_distribution<float> distribution(min, max);
  return distribution(generator);
}

// Function to generate a random position on a torus
glm::vec3 generateRandomTorusPosition(float outerRadius, float innerRadius) {
  float u = randomFloat(0.0f, 2.0f * glm::pi<float>());
  float v = randomFloat(0.0f, 2.0f * glm::pi<float>());
  float x = (outerRadius + innerRadius * cos(v)) * cos(u);
  float z = (outerRadius + innerRadius * cos(v)) * sin(u); // swapped y with z
  float y = innerRadius * sin(v) / 4;                      // swapped z with y
  return glm::vec3(x, y, z);
}

MainRenderer::MainRenderer() {}

MainRenderer::MainRenderer(EngineCore *pEngineCore) {
  Init_MainRenderer(pEngineCore);
}

void MainRenderer::Init_MainRenderer(EngineCore *pEngineCore) {

  // init core pointer
  this->pEngineCore = pEngineCore;

  // shader
  shader = gtp::Shader(pEngineCore);

  // particle
  // this->assets.particle = new gtp::Particle(pEngineCore);

  // load assets
  this->LoadAssets();

  std::cout << "\nfinished loading assets." << std::endl;

  ////create geometry nodes buffer
  this->CreateGeometryNodesBuffer();

  std::cout << "\nfinished creating geometry nodes buffers." << std::endl;

  ////create top level acceleration structure
  this->CreateTLAS();

  std::cout << "\nfinished creating top level acceleration structure"
            << std::endl;

  ////create storage image
  this->CreateStorageImages();

  std::cout << "\nfinished creating storage image" << std::endl;

  // create color id image buffer
  this->CreateColorIDImageBuffer();

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

std::vector<ComputeVertex *> MainRenderer::GetComputeInstances() {
  return this->gltfCompute;
}

void MainRenderer::SetupModelDataTransforms(
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

void MainRenderer::LoadModel(
    std::string filename, uint32_t fileLoadingFlags,
    Utilities_Renderer::ModelLoadingFlags modelLoadingFlags,
    Utilities_UI::TransformMatrices *pTransformMatrices) {

  // model instance pointer to initialize and add to list
  auto *tempModel = new gtp::Model();

  // model index
  auto modelIdx = static_cast<int>(this->assets.models.size());

  // -- Load From File ---- gtp::Model function
  tempModel->loadFromFile(filename, pEngineCore, pEngineCore->queue.graphics,
                          fileLoadingFlags);

  // set semi transparent flag
  const bool isSemiTransparent =
      modelLoadingFlags ==
              Utilities_Renderer::ModelLoadingFlags::SemiTransparent
          ? 1
          : 0;

  tempModel->semiTransparentFlag = static_cast<int>(isSemiTransparent);

  // Set "isAnimated" flag and add to list
  // referenced by blas/tlas/compute vertex
  const bool isAnimated = tempModel->animations.empty() ? false : true;

  this->assets.modelData.animatedModelIndex.push_back(
      static_cast<int>(isAnimated));

  // add animated toggle to list
  this->assets.modelData.isAnimated.push_back(false);

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
  CreateBLAS(tempModel);

  // not a particle
  this->assets.particle.push_back(nullptr);
}

void MainRenderer::LoadParticle(
    std::string filename, uint32_t fileLoadingFlags,
    Utilities_Renderer::ModelLoadingFlags modelLoadingFlags,
    Utilities_UI::TransformMatrices *pTransformMatrices) {

  // model index
  auto modelIdx = static_cast<int>(this->assets.models.size());

  // particle instance pointer to initialize and add to list
  // this->assets.particle = new gtp::Particle(this->pEngineCore);
  auto tempParticle = new gtp::Particle(this->pEngineCore);

  // set semi transparent flag
  const bool isSemiTransparent =
      modelLoadingFlags ==
              Utilities_Renderer::ModelLoadingFlags::SemiTransparent
          ? 1
          : 0;

  tempParticle->ParticleModel()->semiTransparentFlag =
      static_cast<int>(isSemiTransparent);

  tempParticle->ParticleModel()->isParticle = true;

  // Set "isAnimated" flag and add to list
  // referenced by blas/tlas/compute vertex
  const bool isAnimated =
      !tempParticle->ParticleModel()->animations.empty() ? true : false;

  this->assets.modelData.animatedModelIndex.push_back(
      static_cast<int>(isAnimated));

  // add animated toggle to list
  this->assets.modelData.isAnimated.push_back(false);

  // add active animation and animation name to their lists.
  // assign 0 and "none" if model is not animated
  std::vector<std::string> tempNames;
  std::vector<int> tempAnimationIndex;

  if (tempParticle->ParticleModel()->animations.empty()) {
    tempAnimationIndex.push_back(0);
    this->assets.modelData.activeAnimation.push_back(tempAnimationIndex);
    tempNames.push_back("none");
    this->assets.modelData.animationNames.push_back(tempNames);
  }

  else {
    tempAnimationIndex.push_back(0);
    this->assets.modelData.activeAnimation.push_back(tempAnimationIndex);
    for (int i = 0; i < tempParticle->ParticleModel()->animations.size(); i++) {
      tempNames.push_back(tempParticle->ParticleModel()->animations[i].name);
      std::cout << tempParticle->ParticleModel()->animations[i].name
                << std::endl;
    }
    this->assets.modelData.animationNames.push_back(tempNames);
  }

  // add model to model list
  this->assets.models.push_back(tempParticle->ParticleModel());

  // init modelData struct
  this->assets.modelData.modelName.push_back(
      tempParticle->ParticleModel()->modelName);

  // matrices
  Utilities_UI::TransformMatrices transformMatrices{};

  // vectors of xyzw values
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

  // load gltf -- internally conditional if model contains animations else ==
  // nullptr
  LoadGltfCompute(tempParticle->ParticleModel());

  // create bottom level acceleration structure for model
  CreateBLAS(tempParticle->ParticleModel());

  this->assets.particle.push_back(tempParticle);
}

void MainRenderer::LoadAssets() {

  // cubemap and default textures
  this->assets.cubemap = gtp::TextureLoader(this->pEngineCore);
  this->assets.cubemap.LoadCubemap();

  // -- load glass texture just because why not load a ktx
  this->assets.coloredGlassTexture = gtp::TextureLoader(this->pEngineCore);

  this->assets.coloredGlassTexture.loadFromFile(
      "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/"
      "textures/"
      "colored_glass_rgba.ktx",
      VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT,
      VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

  this->assets.defaultTextures.push_back(this->assets.cubemap);

  // update texture offset for shader
  this->assets.textureOffset =
      static_cast<uint32_t>(this->assets.defaultTextures.size());

  const uint32_t glTFLoadingFlags =
      gtp::FileLoadingFlags::PreTransformVertices |
      gtp::FileLoadingFlags::PreMultiplyVertexColors;

  // -- load animation
  Utilities_UI::TransformMatrices animatedTransformMatrices{};

  animatedTransformMatrices.rotate = glm::rotate(
      glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

  animatedTransformMatrices.translate =
      glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.0f, 0.0f));

  animatedTransformMatrices.scale =
      glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

  this->LoadModel("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/"
                  "assets/models/Fox2/Fox2.gltf",
                  gtp::FileLoadingFlags::None,
                  Utilities_Renderer::ModelLoadingFlags::Animated,
                  &animatedTransformMatrices);

  // -- load scene
  this->LoadModel(
      "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/"
      "desert_scene/testScene.gltf",
      gtp::FileLoadingFlags::None, Utilities_Renderer::ModelLoadingFlags::None,
      nullptr);

  // -- load water surface
  this->LoadModel(
      "C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/models/"
      "desert_scene/pool_water_surface/pool_water_surface.gltf",
      gtp::FileLoadingFlags::None,
      Utilities_Renderer::ModelLoadingFlags::SemiTransparent, nullptr);

  // -- load particle
  this->LoadParticle("", gtp::FileLoadingFlags::None,
                     Utilities_Renderer::ModelLoadingFlags::None, nullptr);

  for (int i = 0; i < this->assets.modelData.animatedModelIndex.size(); i++) {
    std::cout << "\nanimated model index[" << i
              << "]: " << this->assets.modelData.animatedModelIndex[i]
              << std::endl;
  }
  std::cout << "this->assets.modelData.animatedModelIndex.size(): "
            << this->assets.modelData.animatedModelIndex.size() << std::endl;
}

void MainRenderer::LoadGltfCompute(gtp::Model *pModel) {

  ComputeVertex *computeVtx = nullptr;

  if (!pModel->animations.empty()) {
    computeVtx = new ComputeVertex(pEngineCore, pModel);
  }

  this->gltfCompute.push_back(computeVtx);
}

void MainRenderer::CreateBLAS(gtp::Model *pModel) {

  // new blas instance
  Utilities_AS::BLASData *tempBLAS = new Utilities_AS::BLASData();

  // create blas
  Utilities_AS::createBLAS(this->pEngineCore, &this->geometryNodeBuf,
                           &this->geometryIndexBuf, tempBLAS,
                           &tempBLAS->accelerationStructure, pModel,
                           this->assets.textureOffset);

  // increment offset by size of models texture array
  this->assets.textureOffset += static_cast<uint32_t>(pModel->textures.size());

  // add blas to renderer's blas 'buffer'
  this->bottomLevelAccelerationStructures.push_back(tempBLAS);
}

void MainRenderer::CreateTLAS() {

  // blas instances buffer size decl.
  VkDeviceSize blasInstancesBufSize = 0;

  // void pointer for blas instances data
  void *blasInstancesData = nullptr;

  // resize renderer's blas instances 'buffer'
  blasInstances.resize(this->bottomLevelAccelerationStructures.size());

  // if blas instances 'buffer' contains at least one element
  if (blasInstances.size() != 0) {

    // iterate through the models list
    for (int i = 0; i < this->assets.models.size(); i++) {

      // handle the instances for particles if the model was loaded for use as a
      // particle
      if (i > 0 && this->assets.models[i]->isParticle) {
        this->InitializeParticleBLASInstances(i);
      }

      // otherwise handle each instance
      else {

        /* transform matrices */
        // rotation matrix
        glm::mat4 rotationMatrix =
            this->assets.modelData.transformMatrices[i].rotate;

        // translation matrix
        glm::mat4 translationMatrix =
            this->assets.modelData.transformMatrices[i].translate;

        // scale matrix
        glm::mat4 scaleMatrix =
            this->assets.modelData.transformMatrices[i].scale;

        // Combine rotation, translation, and scale into a single 4x4 matrix
        glm::mat4 transformMatrix =
            translationMatrix * rotationMatrix * scaleMatrix;

        // Convert glm::mat4 to VkTransformMatrixKHR
        VkTransformMatrixKHR vkTransformMatrix;

        for (int col = 0; col < 4; ++col) {
          for (int row = 0; row < 3; ++row) {
            vkTransformMatrix.matrix[row][col] =
                transformMatrix[col][row]; // Vulkan expects column-major order
          }
        }

        // assign VkTransformMatrixKHR to instance and initialize instance
        // struct data
        blasInstances[i].transform = vkTransformMatrix;
        blasInstances[i].instanceCustomIndex = i;
        blasInstances[i].mask = 0xFF;
        blasInstances[i].instanceShaderBindingTableRecordOffset = 0;
        blasInstances[i].accelerationStructureReference =
            this->bottomLevelAccelerationStructures[i]
                ->accelerationStructure.deviceAddress;
        std::cout << "this->secondBLAS.deviceAddress"
                  << this->bottomLevelAccelerationStructures[i]
                         ->accelerationStructure.deviceAddress
                  << std::endl;
      }
    }
  }

  /* instances buffer */
  // name buffer for debug hashmap
  buffers.tlas_instancesBuffer.bufferData.bufferName =
      "MainRenderer_TLASInstancesBuffer";
  buffers.tlas_instancesBuffer.bufferData.bufferMemoryName =
      "MainRenderer_TLASInstancesBufferMemory";

  // if blas instances 'buffer' has any elements
  if (blasInstances.size() != 0) {

    // size of instance struct * number of instances in the 'buffer'
    blasInstancesBufSize = sizeof(VkAccelerationStructureInstanceKHR) *
                           static_cast<uint32_t>(blasInstances.size());

    // void pointer now points to instances 'buffer' data
    blasInstancesData = blasInstances.data();
  }

  // if instances 'buffer' is empty, assign a single uninitialized AS instance
  // struct so buffer size is not zero and can be read/written to
  else {
    blasInstancesBufSize = sizeof(VkAccelerationStructureInstanceKHR);
  }

  // create instances buffer
  if (pEngineCore->CreateBuffer(
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
              VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
          &buffers.tlas_instancesBuffer, blasInstancesBufSize,
          blasInstancesData) != VK_SUCCESS) {
    throw std::invalid_argument(
        "failed to create MainRenderer instances buffer");
  }

  // get instance buffer device address
  tlasData.instanceDataDeviceAddress.deviceAddress =
      Utilities_AS::getBufferDeviceAddress(
          this->pEngineCore, buffers.tlas_instancesBuffer.bufferData.buffer);

  // initialize top level acceleration structure geometry struct
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

  /* get tlas size information */
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
      &tlasData.accelerationStructureBuildSizesInfo, "mainRenderer_TLASBuffer");

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
      "mainRenderer_ScratchBufferTLAS");

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

void MainRenderer::InitializeParticleBLASInstances(int particleIdx) {
  for (int i = 0; i < PARTICLE_COUNT; i++) {

    VkAccelerationStructureInstanceKHR particleInstance;

    glm::mat4 rotationMatrix =
        this->assets.modelData.transformMatrices[particleIdx].rotate;

    // Scale down to 10%
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));

    // Generate a random position on the torus
    glm::vec3 randomPosition = generateRandomTorusPosition(1000.0f, 450.0f);

    // Apply translation
    glm::mat4 translationMatrix =
        glm::translate(glm::mat4(1.0f), randomPosition);

    // Combine rotation, translation, and scale into a single 4x4 matrix
    glm::mat4 transformMatrix =
        translationMatrix * rotationMatrix * scaleMatrix;

    // Convert glm::mat4 to VkTransformMatrixKHR
    VkTransformMatrixKHR vkTransformMatrix;

    for (int col = 0; col < 4; ++col) {
      for (int row = 0; row < 3; ++row) {
        vkTransformMatrix.matrix[row][col] =
            transformMatrix[col][row]; // Vulkan expects column-major order
      }
    }

    // Assign this VkTransformMatrixKHR to your instance
    particleInstance.transform = vkTransformMatrix;
    particleInstance.instanceCustomIndex = particleIdx;
    particleInstance.mask = 0xFF;
    particleInstance.instanceShaderBindingTableRecordOffset = 0;
    particleInstance.accelerationStructureReference =
        this->bottomLevelAccelerationStructures[particleIdx]
            ->accelerationStructure.deviceAddress;
    // std::cout << "this->secondBLAS.deviceAddress"
    //   << this->bottomLevelAccelerationStructures[particleIdx]
    //   ->accelerationStructure.deviceAddress
    //   << std::endl;
    blasInstances.push_back(particleInstance);
  }
}

void MainRenderer::CreateStorageImages() {

  // color storage image
  Utilities_AS::createStorageImage(this->pEngineCore, &this->storageImage,
                                   "mainRenderer_storageImage");

  // color id storage image
  Utilities_AS::createStorageImage(this->pEngineCore,
                                   &this->colorIDStorageImage,
                                   "mainRenderer_colorIDStorageImage");
}

void MainRenderer::CreateColorIDImageBuffer() {
  // name color image id buffer
  buffers.colorIDImageBuffer.bufferData.bufferName = "Color_Image_ID_Buffer";
  buffers.colorIDImageBuffer.bufferData.bufferMemoryName =
      "Color_Image_ID_BufferMemory";

  // buffer size
  VkDeviceSize bufferSize =
      pEngineCore->swapchainData.swapchainExtent2D.width *
      pEngineCore->swapchainData.swapchainExtent2D.height * 4;

  // create color id image buffer
  validate_vk_result(pEngineCore->CreateBuffer(
      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
          VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &buffers.colorIDImageBuffer, bufferSize, nullptr));
}

void MainRenderer::RetrieveObjectIDFromImage() {

  if (this->pEngineCore->posX <
          this->pEngineCore->swapchainData.swapchainExtent2D.width &&
      this->pEngineCore->posX > 0) {
    if (this->pEngineCore->posY <
            this->pEngineCore->swapchainData.swapchainExtent2D.height &&
        this->pEngineCore->posY > 0) {

      VkCommandBuffer commandBuffer =
          pEngineCore->objCreate.VKCreateCommandBuffer(
              VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

      // subresource range
      VkImageSubresourceRange subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0,
                                                  1, 0, 1};

      // set color ID image layout to transfer src optimal
      gtp::Utilities_EngCore::setImageLayout(
          commandBuffer, colorIDStorageImage.image, VK_IMAGE_LAYOUT_GENERAL,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, subresourceRange);

      // copy region
      VkBufferImageCopy region{};
      region.bufferOffset = 0;
      region.bufferRowLength = 0;
      region.bufferImageHeight = 0;
      region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      region.imageSubresource.mipLevel = 0;
      region.imageSubresource.baseArrayLayer = 0;
      region.imageSubresource.layerCount = 1;
      region.imageOffset = {0, 0, 0};
      region.imageExtent = {pEngineCore->swapchainData.swapchainExtent2D.width,
                            pEngineCore->swapchainData.swapchainExtent2D.height,
                            1};

      vkCmdCopyImageToBuffer(commandBuffer, colorIDStorageImage.image,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             this->buffers.colorIDImageBuffer.bufferData.buffer,
                             1, &region);

      // Map the buffer memory
      void *data;
      vkMapMemory(pEngineCore->devices.logical,
                  this->buffers.colorIDImageBuffer.bufferData.memory, 0,
                  VK_WHOLE_SIZE, 0, &data);

      // Adjust mouse coordinates if necessary
      auto adjustedY =
          static_cast<int>(pEngineCore->swapchainData.swapchainExtent2D.height -
                           1 - this->pEngineCore->posY);

      // Calculate the index
      int index = static_cast<int>(
          (adjustedY * pEngineCore->swapchainData.swapchainExtent2D.width +
           this->pEngineCore->posX) *
          4);

      // Retrieve the color at the mouse position
      uint8_t *pixel = static_cast<uint8_t *>(data) + index;
      uint8_t red = pixel[0];
      uint8_t green = pixel[1];
      uint8_t blue = pixel[2];
      uint8_t alpha = pixel[3];

      // Unmap the buffer memory
      vkUnmapMemory(pEngineCore->devices.logical,
                    this->buffers.colorIDImageBuffer.bufferData.memory);

      // Identify the object using the color
      float objectID = red;

      // std::cout << "Selected Object ID: " << objectID << std::endl;

      // set color ID image layout to transfer src optimal
      gtp::Utilities_EngCore::setImageLayout(
          commandBuffer, colorIDStorageImage.image,
          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
          subresourceRange);

      // end and submit and destroy command buffer
      pEngineCore->FlushCommandBuffer(commandBuffer,
                                      pEngineCore->queue.graphics,
                                      pEngineCore->commandPools.graphics, true);
    }
  }
}

void MainRenderer::CreateUniformBuffer() {

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

  UpdateUniformBuffer(0.0f, glm::vec4(0.0f));
}

void MainRenderer::UpdateUniformBuffer(float deltaTime,
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

void MainRenderer::CreateRayTracingPipeline() {

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
  pEngineCore->add(
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
  pEngineCore->add(
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
    this->shaderBindingTableData.shaderGroups.push_back(shaderGroup);
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
    this->shaderBindingTableData.shaderGroups.push_back(shaderGroup);
    // Second shader for glTFShadows
    shaderStages.push_back(shader.loadShader(
        projDirectory.string() +
            "/shaders/compiled/main_renderer_shadow.rmiss.spv",
        VK_SHADER_STAGE_MISS_BIT_KHR, "main_renderer_miss_shadow"));
    shaderGroup.generalShader = static_cast<uint32_t>(shaderStages.size()) - 1;
    this->shaderBindingTableData.shaderGroups.push_back(shaderGroup);
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
    this->shaderBindingTableData.shaderGroups.push_back(shaderGroup);
  }

  //	Create the ray tracing pipeline
  VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo{};
  rayTracingPipelineCreateInfo.sType =
      VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
  rayTracingPipelineCreateInfo.stageCount =
      static_cast<uint32_t>(shaderStages.size());
  rayTracingPipelineCreateInfo.pStages = shaderStages.data();
  rayTracingPipelineCreateInfo.groupCount =
      static_cast<uint32_t>(this->shaderBindingTableData.shaderGroups.size());
  rayTracingPipelineCreateInfo.pGroups =
      this->shaderBindingTableData.shaderGroups.data();
  rayTracingPipelineCreateInfo.maxPipelineRayRecursionDepth = 1;
  rayTracingPipelineCreateInfo.layout = pipelineData.pipelineLayout;

  // create raytracing pipeline
  pEngineCore->add(
      [this, &rayTracingPipelineCreateInfo]() {
        return pEngineCore->objCreate.VKCreateRaytracingPipeline(
            &rayTracingPipelineCreateInfo, nullptr, &pipelineData.pipeline);
      },
      "mainRenderer_RaytracingPipeline");
}

void MainRenderer::UpdateRayTracingPipeline() {
  // -- pipeline and layout
  vkDestroyPipeline(this->pEngineCore->devices.logical,
                    this->pipelineData.pipeline, nullptr);
  vkDestroyPipelineLayout(this->pEngineCore->devices.logical,
                          this->pipelineData.pipelineLayout, nullptr);

  // descriptor pool
  vkDestroyDescriptorPool(this->pEngineCore->devices.logical,
                          this->pipelineData.descriptorPool, nullptr);

  // -- descriptor set layout
  vkDestroyDescriptorSetLayout(this->pEngineCore->devices.logical,
                               this->pipelineData.descriptorSetLayout, nullptr);

  this->CreateRayTracingPipeline();
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
  const uint32_t groupCount =
      static_cast<uint32_t>(this->shaderBindingTableData.shaderGroups.size());

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
    this->shaderBindingTableData.raygenShaderBindingTable.bufferData
        .bufferName = "RaygenShaderBindingTable";
    this->shaderBindingTableData.raygenShaderBindingTable.bufferData
        .bufferMemoryName = "RaygenShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(
            bufferUsageFlags, memoryUsageFlags,
            &this->shaderBindingTableData.raygenShaderBindingTable, handleSize,
            nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create raygenShaderBindingTable");
    }

    // miss
    this->shaderBindingTableData.missShaderBindingTable.bufferData.bufferName =
        "MissShaderBindingTable";
    this->shaderBindingTableData.missShaderBindingTable.bufferData
        .bufferMemoryName = "MissShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(
            bufferUsageFlags, memoryUsageFlags,
            &this->shaderBindingTableData.missShaderBindingTable,
            handleSize * 2, nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create missShaderBindingTable");
    }

    // hit
    this->shaderBindingTableData.hitShaderBindingTable.bufferData.bufferName =
        "HitShaderBindingTable";
    this->shaderBindingTableData.hitShaderBindingTable.bufferData
        .bufferMemoryName = "HitShaderBindingTableMemory";
    if (pEngineCore->CreateBuffer(
            bufferUsageFlags, memoryUsageFlags,
            &this->shaderBindingTableData.hitShaderBindingTable, handleSize * 2,
            nullptr) != VK_SUCCESS) {
      throw std::runtime_error("failed to create hitShaderBindingTable");
    }

    // copy buffers
    this->shaderBindingTableData.raygenShaderBindingTable.map(
        pEngineCore->devices.logical);
    memcpy(
        this->shaderBindingTableData.raygenShaderBindingTable.bufferData.mapped,
        shaderHandleStorage.data(), handleSize);
    this->shaderBindingTableData.raygenShaderBindingTable.unmap(
        pEngineCore->devices.logical);

    this->shaderBindingTableData.missShaderBindingTable.map(
        pEngineCore->devices.logical);
    memcpy(
        this->shaderBindingTableData.missShaderBindingTable.bufferData.mapped,
        shaderHandleStorage.data() + handleSizeAligned, handleSize * 2);
    this->shaderBindingTableData.missShaderBindingTable.unmap(
        pEngineCore->devices.logical);

    this->shaderBindingTableData.hitShaderBindingTable.map(
        pEngineCore->devices.logical);
    memcpy(this->shaderBindingTableData.hitShaderBindingTable.bufferData.mapped,
           shaderHandleStorage.data() + handleSizeAligned * 3, handleSize * 2);
    this->shaderBindingTableData.hitShaderBindingTable.unmap(
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

void MainRenderer::CreateDescriptorSet() {

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
  pEngineCore->add(
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
  pEngineCore->add(
      [this, &descriptorSetAllocateInfo]() {
        return pEngineCore->objCreate.VKAllocateDescriptorSet(
            &descriptorSetAllocateInfo, nullptr,
            &this->pipelineData.descriptorSet);
      },
      "mainRenderer_DescriptorSet");

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

  VkDescriptorImageInfo colorIDStorageImageDescriptor{
      VK_NULL_HANDLE, colorIDStorageImage.view, VK_IMAGE_LAYOUT_GENERAL};

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

  // g_nodes_buffer
  VkDescriptorBufferInfo g_nodes_BufferDescriptor{
      this->buffers.g_nodes_buffer.bufferData.buffer, 0,
      this->buffers.g_nodes_buffer.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet g_nodes_bufferWrite{};
  g_nodes_bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  g_nodes_bufferWrite.dstSet = pipelineData.descriptorSet;
  g_nodes_bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  g_nodes_bufferWrite.dstBinding = 4;
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
  g_nodes_indicesWrite.dstBinding = 5;
  g_nodes_indicesWrite.pBufferInfo = &g_nodes_indicesDescriptor;
  g_nodes_indicesWrite.descriptorCount = 1;

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
      // Binding 3: g_nodes_buffer write
      g_nodes_bufferWrite,
      // Binding 4: g_nodes_indices write
      g_nodes_indicesWrite,
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

void MainRenderer::SetupBufferRegionAddresses() {

  // setup buffer regions pointing to shaders in shader binding table
  const uint32_t handleSizeAligned = gtp::Utilities_EngCore::alignedSize(
      pEngineCore->deviceProperties.rayTracingPipelineKHR.shaderGroupHandleSize,
      pEngineCore->deviceProperties.rayTracingPipelineKHR
          .shaderGroupHandleAlignment);

  // VkStridedDeviceAddressRegionKHR raygenShaderSbtEntry{};
  this->shaderBindingTableData.raygenStridedDeviceAddressRegion
      .deviceAddress = Utilities_AS::getBufferDeviceAddress(
      this->pEngineCore,
      this->shaderBindingTableData.raygenShaderBindingTable.bufferData.buffer);
  this->shaderBindingTableData.raygenStridedDeviceAddressRegion.stride =
      handleSizeAligned;
  this->shaderBindingTableData.raygenStridedDeviceAddressRegion.size =
      handleSizeAligned;

  // VkStridedDeviceAddressRegionKHR missShaderSbtEntry{};
  this->shaderBindingTableData.missStridedDeviceAddressRegion
      .deviceAddress = Utilities_AS::getBufferDeviceAddress(
      this->pEngineCore,
      this->shaderBindingTableData.missShaderBindingTable.bufferData.buffer);
  this->shaderBindingTableData.missStridedDeviceAddressRegion.stride =
      handleSizeAligned;
  this->shaderBindingTableData.missStridedDeviceAddressRegion.size =
      handleSizeAligned * 2;

  // VkStridedDeviceAddressRegionKHR hitShaderSbtEntry{};
  this->shaderBindingTableData.hitStridedDeviceAddressRegion.deviceAddress =
      Utilities_AS::getBufferDeviceAddress(
          this->pEngineCore,
          this->shaderBindingTableData.hitShaderBindingTable.bufferData.buffer);
  this->shaderBindingTableData.hitStridedDeviceAddressRegion.stride =
      handleSizeAligned;
  this->shaderBindingTableData.hitStridedDeviceAddressRegion.size =
      handleSizeAligned * 2;
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
        &this->shaderBindingTableData.raygenStridedDeviceAddressRegion,
        &this->shaderBindingTableData.missStridedDeviceAddressRegion,
        &this->shaderBindingTableData.hitStridedDeviceAddressRegion,
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

void MainRenderer::RebuildCommandBuffers(int frame, bool showObjectColorID) {

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
      &this->shaderBindingTableData.raygenStridedDeviceAddressRegion,
      &this->shaderBindingTableData.missStridedDeviceAddressRegion,
      &this->shaderBindingTableData.hitStridedDeviceAddressRegion,
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
        pEngineCore->commandBuffers.graphics[frame], storageImage.image,
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

  else {
    gtp::Utilities_EngCore::setImageLayout(
        pEngineCore->commandBuffers.graphics[frame], colorIDStorageImage.image,
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
                   colorIDStorageImage.image,
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
        pEngineCore->commandBuffers.graphics[frame], colorIDStorageImage.image,
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL,
        subresourceRange);
  }
}

void MainRenderer::UpdateBLAS() {
  // Build
  // via a one-time command buffer submission
  // std::cout << "update blas test 1" << std::endl;

  VkCommandBuffer commandBuffer = pEngineCore->objCreate.VKCreateCommandBuffer(
      VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
  // handle animated blas
  for (int i = 0; i < this->assets.models.size(); i++) {

    if (!this->assets.modelData.animatedModelIndex.empty()) {
      if (this->assets.modelData.animatedModelIndex[i] == 1) {
        // verify current model is being animated
        if (this->assets.modelData.isAnimated[i]) {
          // std::cout << "update blas test 2[" << i << "]" << std::endl;
          // std::cout << "test" << std::endl;
          //  build BLAS
          pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
              commandBuffer, 1,
              &this->bottomLevelAccelerationStructures[i]
                   ->accelerationStructureBuildGeometryInfo,
              this->bottomLevelAccelerationStructures[i]
                  ->pBuildRangeInfos.data());

          updateTLAS = true;
        }
      }
    }
  }

  // handle non animated blas
  for (int i = 0; i < this->assets.modelData.updateBLAS.size(); i++) {
    // std::cout << "update blas test 3[" << i << "]" << std::endl;
    if (this->assets.modelData.updateBLAS[i] == 1 &&
        this->assets.modelData.animatedModelIndex[i] != 1) {
      // build BLAS
      pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
          commandBuffer, 1,
          &this->bottomLevelAccelerationStructures[i]
               ->accelerationStructureBuildGeometryInfo,
          this->bottomLevelAccelerationStructures[i]->pBuildRangeInfos.data());
      // this->assets.modelData.updateBLAS[i] = false;
      updateTLAS = true;
    }
  }

  // std::cout << "update blas test 4" << std::endl;

  // end and submit and destroy command buffer
  pEngineCore->FlushCommandBuffer(commandBuffer, pEngineCore->queue.graphics,
                                  pEngineCore->commandPools.graphics, true);

  // std::cout << "update blas test 5" << std::endl;

  // std::cout << "this->BLAS.deviceAddress" << this->BLAS.deviceAddress <<
  // std::endl;
}

void MainRenderer::UpdateTLAS() {
  if (this->updateTLAS) {
    // initialize instances array
    if (blasInstances.size() != 0) {
      for (int i = 0; i < this->assets.modelData.updateBLAS.size(); i++) {
        if (this->assets.modelData.updateBLAS[i]) {

          glm::mat4 rotationMatrix =
              this->assets.modelData.transformMatrices[i].rotate;
          glm::mat4 translationMatrix =
              this->assets.modelData.transformMatrices[i].translate;

          glm::mat4 scaleMatrix =
              this->assets.modelData.transformMatrices[i].scale;

          // Combine rotation, translation, and scale into a single 4x4 matrix
          glm::mat4 transformMatrix =
              translationMatrix * rotationMatrix * scaleMatrix;

          // Convert glm::mat4 to VkTransformMatrixKHR
          VkTransformMatrixKHR vkTransformMatrix;
          for (int col = 0; col < 4; ++col) {
            for (int row = 0; row < 3; ++row) {
              vkTransformMatrix.matrix[row][col] =
                  transformMatrix[col]
                                 [row]; // Vulkan expects column-major order
            }
          }

          // this->blasInstances[i]// Assign this VkTransformMatrixKHR to your
          // instance
          blasInstances[i].transform = vkTransformMatrix;
          blasInstances[i].instanceCustomIndex = i;
          blasInstances[i].mask = 0xFF;
          blasInstances[i].instanceShaderBindingTableRecordOffset = 0;
          blasInstances[i].accelerationStructureReference =
              this->bottomLevelAccelerationStructures[i]
                  ->accelerationStructure.deviceAddress;
        }
      }
    }

    if (blasInstances.size() != 0) {

      // -- update instances buffer
      buffers.tlas_instancesBuffer.copyTo(
          blasInstances.data(),
          sizeof(VkAccelerationStructureInstanceKHR) *
              static_cast<uint32_t>(blasInstances.size()));

      // -- instance buffer device address
      tlasData.instanceDataDeviceAddress.deviceAddress =
          Utilities_AS::getBufferDeviceAddress(
              this->pEngineCore,
              buffers.tlas_instancesBuffer.bufferData.buffer);

      // -- acceleration Structure Geometry{};
      tlasData.accelerationStructureGeometry.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      tlasData.accelerationStructureGeometry.geometryType =
          VK_GEOMETRY_TYPE_INSTANCES_KHR;
      tlasData.accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      tlasData.accelerationStructureGeometry.geometry.instances.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      tlasData.accelerationStructureGeometry.geometry.instances
          .arrayOfPointers = VK_FALSE;
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
          &tlasData.primitive_count,
          &tlasData.accelerationStructureBuildSizesInfo);

      VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
      accelerationStructureGeometry.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
      accelerationStructureGeometry.geometryType =
          VK_GEOMETRY_TYPE_INSTANCES_KHR;
      accelerationStructureGeometry.flags = VK_GEOMETRY_OPAQUE_BIT_KHR;
      accelerationStructureGeometry.geometry.instances.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
      accelerationStructureGeometry.geometry.instances.arrayOfPointers =
          VK_FALSE;
      accelerationStructureGeometry.geometry.instances.data =
          tlasData.instanceDataDeviceAddress;

      // Get size info
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
      accelerationStructureBuildRangeInfo.primitiveCount =
          tlasData.primitive_count;
      accelerationStructureBuildRangeInfo.primitiveOffset = 0;
      accelerationStructureBuildRangeInfo.firstVertex = 0;
      accelerationStructureBuildRangeInfo.transformOffset = 0;

      std::vector<VkAccelerationStructureBuildRangeInfoKHR *>
          accelerationBuildStructureRangeInfos = {
              &accelerationStructureBuildRangeInfo};

      // build the acceleration structure on the device via a one-time command
      // buffer submission some implementations may support acceleration
      // structure building on the host
      //(VkPhysicalDeviceAccelerationStructureFeaturesKHR->accelerationStructureHostCommands),
      // but we prefer device builds create command buffer
      VkCommandBuffer commandBuffer =
          pEngineCore->objCreate.VKCreateCommandBuffer(
              VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

      // this is where i start next
      // build acceleration structures
      pEngineCore->coreExtensions->vkCmdBuildAccelerationStructuresKHR(
          commandBuffer, 1, &accelerationStructureBuildGeometryInfo,
          accelerationBuildStructureRangeInfos.data());

      // flush command buffer
      pEngineCore->FlushCommandBuffer(commandBuffer,
                                      pEngineCore->queue.graphics,
                                      pEngineCore->commandPools.graphics, true);

      // std::cout << " test" << std::endl;

      // get acceleration structure device address
      VkAccelerationStructureDeviceAddressInfoKHR
          accelerationDeviceAddressInfo{};
      accelerationDeviceAddressInfo.sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
      accelerationDeviceAddressInfo.accelerationStructure =
          this->TLAS.accelerationStructureKHR;

      this->TLAS.deviceAddress =
          pEngineCore->coreExtensions
              ->vkGetAccelerationStructureDeviceAddressKHR(
                  pEngineCore->devices.logical, &accelerationDeviceAddressInfo);
    }
  }
}

// void MainRenderer::PreTransformModels() {
//
//   Utilities_Renderer::TransformsData transformsData{};
//   transformsData.model = this->assets.models[3];
//   transformsData.translate =
//       glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
//   transformsData.scale = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f));
//
//   //Utilities_Renderer::TransformModelVertices(this->pEngineCore,
//   //                                           &transformsData);
// }

void MainRenderer::CreateGeometryNodesBuffer() {

  buffers.g_nodes_buffer.bufferData.bufferName = "g_nodes_buffer";
  buffers.g_nodes_buffer.bufferData.bufferMemoryName = "g_nodes_bufferMemory";

  void *nodeData = nullptr;
  VkDeviceSize nodeBufSize = 0;

  if (!geometryNodeBuf.empty()) {
    nodeData = geometryNodeBuf.data();
    nodeBufSize = static_cast<uint32_t>(geometryNodeBuf.size()) *
                  sizeof(Utilities_AS::GeometryNode);
  }

  else {
    nodeData = new Utilities_AS::GeometryNode();
    nodeBufSize = sizeof(Utilities_AS::GeometryNode);
  }

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.g_nodes_buffer, nodeBufSize,
                                nodeData) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create g_nodes_buffer");
  }

  buffers.g_nodes_indices.bufferData.bufferName = "g_nodes_indicesBuffer";
  buffers.g_nodes_indices.bufferData.bufferMemoryName =
      "g_nodes_indicesBufferMemory";

  void *gIndexData = nullptr;
  VkDeviceSize gIndexBufSize = 0;

  if (!geometryIndexBuf.empty()) {
    gIndexData = geometryIndexBuf.data();
    gIndexBufSize =
        static_cast<uint32_t>(geometryIndexBuf.size()) * sizeof(int);
  }

  else {
    gIndexData = std::vector<int>(0).data();
    gIndexBufSize = sizeof(int);
  }

  if (pEngineCore->CreateBuffer(VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                                    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                    VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                &buffers.g_nodes_indices, gIndexBufSize,
                                gIndexData) != VK_SUCCESS) {
    throw std::invalid_argument("failed to create g_nodes_index buffer");
  }
}

void MainRenderer::UpdateGeometryNodesBuffer(gtp::Model *pModel) {

  std::cout << " updating geometry nodes buffer" << std::endl;

  std::cout << pModel->modelName << std::endl;

  this->buffers.g_nodes_buffer.destroy(this->pEngineCore->devices.logical);
  this->buffers.g_nodes_indices.destroy(this->pEngineCore->devices.logical);
  CreateGeometryNodesBuffer();
  //// update g nodes buffer with updated vector of g nodes
  // this->buffers.g_nodes_buffer.copyTo(
  //     this->geometryNodeBuf.data(),
  //     static_cast<uint32_t>(geometryNodeBuf.size()) *
  //         sizeof(Utilities_AS::GeometryNode));

  //// update g node indices buffer with updated vector of g node indices
  // this->buffers.g_nodes_indices.copyTo(
  //     this->geometryIndexBuf.data(),
  //     static_cast<uint32_t>(geometryIndexBuf.size()) * sizeof(int));

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

  CreateTLAS();
  UpdateRayTracingPipeline();
  CreateDescriptorSet();
  // UpdateDescriptorSet();
}

void MainRenderer::DeleteModel() {

  // model idx
  int modelIdx = this->assets.modelData.modelIndex;

  std::cout << " deleting model" << std::endl;

  std::cout << "\tname: " << this->assets.models[modelIdx]->modelName
            << std::endl;
  std::cout << "\tindex: " << this->assets.modelData.modelIndex << std::endl;

  // wait for device idle
  vkDeviceWaitIdle(this->pEngineCore->devices.logical);

  // clear blas instances 'buffer'
  blasInstances.clear();

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
  for (int i = 0; i < this->bottomLevelAccelerationStructures.size(); i++) {

    // accel. structure
    pEngineCore->coreExtensions->vkDestroyAccelerationStructureKHR(
        pEngineCore->devices.logical,
        this->bottomLevelAccelerationStructures[i]
            ->accelerationStructure.accelerationStructureKHR,
        nullptr);

    // scratch buffer
    this->bottomLevelAccelerationStructures[i]
        ->accelerationStructure.scratchBuffer.destroy(
            this->pEngineCore->devices.logical);

    // accel structure buffer and memory
    vkDestroyBuffer(pEngineCore->devices.logical,
                    this->bottomLevelAccelerationStructures[i]
                        ->accelerationStructure.buffer,
                    nullptr);
    vkFreeMemory(pEngineCore->devices.logical,
                 this->bottomLevelAccelerationStructures[i]
                     ->accelerationStructure.memory,
                 nullptr);
  }

  // update texture offset
  this->assets.textureOffset =
      static_cast<uint32_t>(this->assets.defaultTextures.size());

  // create vector of blas pointers to reassign mainRenderer member of blas
  // pointers with
  std::vector<Utilities_AS::BLASData *> tempLevelAccelerationStructures;

  // clear g node buffer
  this->geometryNodeBuf.clear();
  // clear g node index buffer
  this->geometryIndexBuf.clear();
  // clear bottom level acceleration structures "buffer"
  this->bottomLevelAccelerationStructures.clear();

  for (int i = 0; i < this->assets.models.size(); i++) {
    CreateBLAS(this->assets.models[i]);
  }

  // update g nodes buffer with updated vector of g nodes
  this->buffers.g_nodes_buffer.copyTo(
      this->geometryNodeBuf.data(),
      static_cast<uint32_t>(geometryNodeBuf.size()) *
          sizeof(Utilities_AS::GeometryNode));

  // update g node indices buffer with updated vector of g node indices
  this->buffers.g_nodes_indices.copyTo(
      this->geometryIndexBuf.data(),
      static_cast<uint32_t>(geometryIndexBuf.size()) * sizeof(int));

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

  if (this->gltfCompute[this->assets.modelData.modelIndex] != nullptr) {
    this->gltfCompute[this->assets.modelData.modelIndex]
        ->Destroy_ComputeVertex();
  }
  if (this->assets.modelData.modelIndex < this->gltfCompute.size()) {
    this->gltfCompute.erase(this->gltfCompute.begin() +
                            this->assets.modelData.modelIndex);
  }

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

  this->assets.modelData.modelIndex = 0;

  vkDeviceWaitIdle(this->pEngineCore->devices.logical);

  UpdateBLAS();
  CreateTLAS();
  UpdateDescriptorSet();

  vkDeviceWaitIdle(this->pEngineCore->devices.logical);

  this->assets.modelData.deleteModel = false;

  std::cout << "model successfully deleted!" << std::endl;
}

void MainRenderer::UpdateDescriptorSet() {

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

  VkDescriptorImageInfo colorIDStorageImageDescriptor{
      VK_NULL_HANDLE, colorIDStorageImage.view, VK_IMAGE_LAYOUT_GENERAL};

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

  // g_nodes_buffer
  VkDescriptorBufferInfo g_nodes_BufferDescriptor{
      this->buffers.g_nodes_buffer.bufferData.buffer, 0,
      this->buffers.g_nodes_buffer.bufferData.size};

  // geometry descriptor write info
  VkWriteDescriptorSet g_nodes_bufferWrite{};
  g_nodes_bufferWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
  g_nodes_bufferWrite.dstSet = pipelineData.descriptorSet;
  g_nodes_bufferWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  g_nodes_bufferWrite.dstBinding = 4;
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
  g_nodes_indicesWrite.dstBinding = 5;
  g_nodes_indicesWrite.pBufferInfo = &g_nodes_indicesDescriptor;
  g_nodes_indicesWrite.descriptorCount = 1;

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
      // Binding 3: g_nodes_buffer write
      g_nodes_bufferWrite,
      // Binding 4: g_nodes_indices write
      g_nodes_indicesWrite,
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

void MainRenderer::UpdateAnimations(float deltaTime) {
  for (size_t i = 0; i < this->assets.models.size(); ++i) {
    // check animated index to check against model list for animated models
    if (this->assets.modelData.animatedModelIndex[i] == 1) {
      // get current model animation
      int activeAnimation = this->assets.modelData.activeAnimation[i][0];
      // verify current model is being animated
      if (this->assets.modelData.isAnimated[i]) {
        this->assets.models[i]->updateAnimation(
            activeAnimation, deltaTime, &this->gltfCompute[i]->jointBuffer);
      }
    }
  }
}

std::vector<VkCommandBuffer> MainRenderer::RecordParticleComputeCommands(
    int currentFrame, std::vector<VkCommandBuffer> computeCommandBuffer) {

  auto &tempCmdBuf = computeCommandBuffer;

  // particle commands
  for (int i = 0; i > this->assets.particle.size(); i++) {
    if (this->assets.particle[i] != nullptr) {
      tempCmdBuf.push_back(
          this->assets.particle[i]->RecordComputeCommands(currentFrame));
    }
  }

  return tempCmdBuf;
}

void MainRenderer::HandleResize() {

  // Delete allocated resources
  vkDestroyImageView(pEngineCore->devices.logical, storageImage.view, nullptr);
  vkDestroyImage(pEngineCore->devices.logical, storageImage.image, nullptr);
  vkFreeMemory(pEngineCore->devices.logical, storageImage.memory, nullptr);

  // Delete allocated resources
  vkDestroyImageView(pEngineCore->devices.logical, colorIDStorageImage.view,
                     nullptr);
  vkDestroyImage(pEngineCore->devices.logical, colorIDStorageImage.image,
                 nullptr);
  vkFreeMemory(pEngineCore->devices.logical, colorIDStorageImage.memory,
               nullptr);

  // -- destroy color id image buffer
  this->buffers.colorIDImageBuffer.destroy(this->pEngineCore->devices.logical);

  // Recreate image
  this->CreateStorageImages();

  // re create color id image buffer
  this->CreateColorIDImageBuffer();

  // Update descriptor
  this->UpdateDescriptorSet();
}

void MainRenderer::HandleLoadModel(gtp::FileLoadingFlags loadingFlags) {
  // call main renderer load model function
  this->LoadModel(this->assets.modelData.loadModelFilepath, loadingFlags);

  // update main renderer geometry nodes buffer
  this->UpdateGeometryNodesBuffer(
      this->assets.models[this->assets.models.size() - 1]);

  // output loaded model name
  std::cout << "loaded model name from engine: "
            << this->assets.models[this->assets.models.size() - 1]->modelName
            << std::endl;

  // set main renderer model data load model flag to false
  this->assets.modelData.loadModel = false;

  // set main renderer model data load model file path to " "
  this->assets.modelData.loadModelFilepath = " ";
}

Utilities_UI::ModelData *MainRenderer::GetModelData() {
  return &this->assets.modelData;
}

void MainRenderer::SetModelData(Utilities_UI::ModelData *pModelData) {
  this->assets.modelData = *pModelData;
  this->assets.modelData.isUpdated = false;
}

// -- public destroy func
// -- call class destroy function for garbage collection
void MainRenderer::Destroy() { this->Destroy_MainRenderer(); }

void MainRenderer::Destroy_MainRenderer() {

  // -- descriptor pool
  vkDestroyDescriptorPool(pEngineCore->devices.logical,
                          this->pipelineData.descriptorPool, nullptr);

  // -- shader binding tables
  this->shaderBindingTableData.raygenShaderBindingTable.destroy(
      pEngineCore->devices.logical);
  this->shaderBindingTableData.hitShaderBindingTable.destroy(
      pEngineCore->devices.logical);
  this->shaderBindingTableData.missShaderBindingTable.destroy(
      pEngineCore->devices.logical);

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

  // -- color id storage image
  vkDestroyImageView(pEngineCore->devices.logical,
                     this->colorIDStorageImage.view, nullptr);
  vkDestroyImage(pEngineCore->devices.logical, this->colorIDStorageImage.image,
                 nullptr);
  vkFreeMemory(pEngineCore->devices.logical, this->colorIDStorageImage.memory,
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
            ->accelerationStructure.accelerationStructureKHR,
        nullptr);

    // scratch buffer
    this->bottomLevelAccelerationStructures[i]
        ->accelerationStructure.scratchBuffer.destroy(
            this->pEngineCore->devices.logical);

    // accel structure buffer and memory
    vkDestroyBuffer(pEngineCore->devices.logical,
                    this->bottomLevelAccelerationStructures[i]
                        ->accelerationStructure.buffer,
                    nullptr);
    vkFreeMemory(pEngineCore->devices.logical,
                 this->bottomLevelAccelerationStructures[i]
                     ->accelerationStructure.memory,
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

  // color id image buffer
  this->buffers.colorIDImageBuffer.destroy(this->pEngineCore->devices.logical);

  // -- compute class
  for (int i = 0; i < this->gltfCompute.size(); i++) {
    if (this->gltfCompute[i] != nullptr) {
      this->gltfCompute[i]->Destroy_ComputeVertex();
    }
  }

  // -- models
  for (int i = 0; i < this->assets.models.size(); i++) {
    if (this->assets.models[i] != nullptr) {
      this->assets.models[i]->destroy(this->pEngineCore->devices.logical);
    }
  }

  this->assets.cubemap.DestroyTextureLoader();
  this->assets.coloredGlassTexture.DestroyTextureLoader();

  // particle
  for (auto &particles : this->assets.particle) {
    if (particles != nullptr) {
      particles->DestroyParticle();
    }
  }
}
