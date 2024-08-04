#include "Engine.hpp"

namespace gtp {

// -- ctor
Engine::Engine() {}

// -- init
VkResult Engine::InitEngine() {
  // init core
  this->InitCore();

  // input class
  auto uniqueInputPtr = std::make_unique<gtp::Input>(this->pEngineCore);
  this->pInput = uniqueInputPtr.release();

  //// init xy pos
  // this->inputPosition.InitializeMousePosition(800.0f, 600.0f);

  auto uniqueCharacterPtr = std::make_unique<gtp::Being>(this->pEngineCore);
  this->beings.character = uniqueCharacterPtr.release();

  // init ui
  this->InitUI();

  // poll events
  glfwPollEvents();
  this->UpdateDeltaTime();
  // this->userInput();
  this->pInput->Process(deltaTime, this->camera);

  // initialize loading screen
  this->loadingScreen = gtp::LoadingScreen(this->pEngineCore);

  // draw loading screen
  if (!glfwWindowShouldClose(CoreGLFWwindow())) {
    this->loadingScreen.Draw(&this->UI, "Loading Renderer...");
  }

  // init renderers
  this->InitRenderers();

  // update loading screen
  if (!glfwWindowShouldClose(CoreGLFWwindow())) {
    this->loadingScreen.Draw(&this->UI, "Successfully Loaded Renderer!");
  }

  // pause for a sec after everything loads to show success message idk
  // mostly a test?
  std::this_thread::sleep_for(std::chrono::seconds(1));

  // set window attributes for renderer
  glfwSetWindowAttrib(this->camera->window, GLFW_DECORATED, GLFW_TRUE);

  // set imgui attributes for renderer
  ImGuiStyle &imguiStyle = ImGui::GetStyle();
  imguiStyle.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.33f);

  // resize window
  this->camera->framebufferResized = true;
  this->HandleResize();

  // Load the model file path from a custom location
  beings.character->LoadModelFilePath(
      Utilities_EngCore::BuildPath("character_save/model_path.json"));

  // Save the model file path to a custom location
  beings.character->modelFilePath = "updatedModel.obj";
  beings.character->SaveModelFilePath(
      Utilities_EngCore::BuildPath("character_save/model_path.json"));

  return VK_SUCCESS;
}

// void Engine::InitXYPos() {
//   lastX = float(this->GetWindowDimensions().width) / 2.0f;
//   lastY = float(this->GetWindowDimensions().height) / 2.0f;
//
//   posX = float(this->GetWindowDimensions().width) / 2.0f;
//   posY = float(this->GetWindowDimensions().height) / 2.0f;
// }

// -- Run
void Engine::Run() {
  // main loop
  while (!glfwWindowShouldClose(CoreGLFWwindow())) {
    // poll events
    glfwPollEvents();

    if (camera->activeWindow) {
      // timer
      this->UpdateDeltaTime();

      // handle input
      // this->userInput();
      this->pInput->Process(deltaTime, this->camera);

      // update ui
      this->UpdateUI();

      // retrieve object id from image
      this->RetrieveColorID();

      // load new models
      this->LoadModel();

      // delete model
      this->DeleteModel();

      // -- if model data in ui has been changed
      this->UpdateRenderer();

      // draw
      this->Draw();

      // set to false - wont update buffers again unless view changes
      pEngineCore->camera->viewUpdated = false;

      // Update Animation Timer
      this->UpdateAnimationTimer();
    }
  }
}

// -- terminate
//@note destroys objects created related to renderers and core
void Engine::Terminate() {
  // wait idle before destroy
  vkDeviceWaitIdle(this->LogicalDevice());

  // loading screen
  this->loadingScreen.DestroyLoadingScreen();

  // ui
  UI.DestroyUI();

  // renderers
  // MainRenderer
  this->renderers.defaultRenderer->Destroy();
  // this->renderers.mainRenderer.Destroy();

  // core
  DestroyCore();
}

void Engine::UpdateDeltaTime() {
  auto now = static_cast<float>(glfwGetTime());
  this->deltaTime = now - this->lastTime;
  this->lastTime = now;
}

void Engine::UpdateAnimationTimer() { this->timer += deltaTime; }

void Engine::UpdateUI() {
  if (this->isUIUpdated) {
    // this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
    this->UI.SetModelData(this->renderers.defaultRenderer->pModelData());
    this->renderers.defaultRenderer->ModelDataSet(&this->UI.modelData);
    // this->renderers.mainRenderer.SetModelData(&this->UI.modelData);
    this->isUIUpdated = false;
  }
}

void Engine::HandleUI() {
  // update UI input
  this->UI.Input(&this->UI.modelData);

  // update UI vertex/index buffers
  this->UI.UpdateBuffers();

  // draw UI
  this->UI.DrawUI(this->GraphicsCommandBuffer(currentFrame), currentFrame);
}

void Engine::RetrieveColorID() {
  if (this->pEngineCore->camera->mouseOnWindow) {
    if (this->pInput->LeftMouseDown()) {
      this->renderers.defaultRenderer->ObjectID(
          static_cast<int>(this->pInput->MousePosition().x),
          static_cast<int>(this->pInput->MousePosition().y));
    }
  }
}

void Engine::LoadModel() {
  // continue if ui load model is true
  if (this->UI.modelData.loadModel) {
    // set main renderer model data to ui model data
    this->renderers.defaultRenderer->ModelDataSet(&this->UI.modelData);
    // set file loading flags
    gtp::FileLoadingFlags loadingFlags =
        this->UI.loadModelFlags.flipY ? gtp::FileLoadingFlags::FlipY
        : gtp::FileLoadingFlags::None |
                this->UI.loadModelFlags.preMultiplyColors
            ? gtp::FileLoadingFlags::PreMultiplyVertexColors
        : gtp::FileLoadingFlags::None | this->UI.loadModelFlags.preTransform
            ? gtp::FileLoadingFlags::PreTransformVertices
            : gtp::FileLoadingFlags::None;

    // handle renderer part of load model
    // this->renderers.mainRenderer.HandleLoadModel(loadingFlags);
    this->renderers.defaultRenderer->LoadNewModel(loadingFlags);

    this->UI.loadModelFlags.flipY = false;
    this->UI.loadModelFlags.preMultiplyColors = false;
    this->UI.loadModelFlags.preTransform = false;
    this->UI.loadModelFlags.loadModelName = "none";

    // set ui model data to main renderer model data
    // this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
    this->UI.SetModelData(this->renderers.defaultRenderer->pModelData());
  }
}

void Engine::DeleteModel() {
  if (this->UI.modelData.deleteModel) {
    // this->renderers.mainRenderer.SetModelData(&this->UI.modelData);
    this->renderers.defaultRenderer->ModelDataSet(&this->UI.modelData);
    // this->renderers.mainRenderer.DeleteModel();
    this->renderers.defaultRenderer->HandleDeleteModel();

    // this->renderers.mainRenderer.assets.modelData.deleteModel = false;
    // this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
    this->UI.SetModelData(this->renderers.defaultRenderer->pModelData());
    this->UI.modelData.isUpdated = true;
  }
}

void Engine::UpdateRenderer() {
  if (this->UI.modelData.isUpdated) {
    // -- update renderer model data struct with ui
    // this->renderers.mainRenderer.SetModelData(&this->UI.modelData);
    this->renderers.defaultRenderer->ModelDataSet(&this->UI.modelData);
    // this->renderers.mainRenderer.assets.modelData.isUpdated = false;
    //  -- update ui data to updated renderer model data with updated flags
    // this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
    this->UI.SetModelData(this->renderers.defaultRenderer->pModelData());
  }
}

void Engine::HandleResize() {
  // if (camera->activeWindow) {
  if (camera->framebufferResized) {
    // wait idle
    vkDeviceWaitIdle(this->LogicalDevice());
    // recreate semaphores, fences
    RecreateSyncObjects();
    // recreate swapchain, swapchain images/views
    RecreateCoreSwapchain();
    // recreate god knows what
    // renderers.mainRenderer.HandleResize();
    // default renderer resize
    this->renderers.defaultRenderer->Resize();
    // recreate ui framebuffers
    UI.RecreateFramebuffers();
    // Flaggy McFlaggerson
    camera->framebufferResized = false;
    // ret
    return;
  }
  //}
}

void Engine::InitRenderers() {
  std::cout << "\ninitializing MainRenderer "
               "class\n'''''''''''''''''''''''''''''''''''''''''''''''\n"
            << std::endl;

  std::cout << "\ninitializing RenderBase "
               "class\n'''''''''''''''''''''''''''''''''''''''''''''''\n"
            << std::endl;

  // outtput to loading screen
  this->loadingScreen.Draw(&this->UI, "Loading Default Renderer");

  auto uniqueDefaultRendererPtr =
      std::make_unique<DefaultRenderer>(this->pEngineCore);
  this->renderers.defaultRenderer = uniqueDefaultRendererPtr.release();

  std::cout << "\nfinished initializing Default Renderer "
               "class\n'''''''''''''''''''''''''''''''''''''''''''''''\n"
            << std::endl;
}

void Engine::InitUI() {
  UI = CoreUI(this->pEngineCore);
  std::cout << "\nfinished initializing UI\n''''''''''''''''''''''''\n"
            << std::endl;
}

void Engine::BeginGraphicsCommandBuffer(int currentFrame) {
  // command buffer begin info
  VkCommandBufferBeginInfo cmdBufInfo{};
  cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  // begin command buffer
  validate_vk_result(vkBeginCommandBuffer(
      this->GraphicsCommandBuffer(currentFrame), &cmdBufInfo));
}

void Engine::EndGraphicsCommandBuffer(int currentFrame) {
  validate_vk_result(
      vkEndCommandBuffer(this->GraphicsCommandBuffer(currentFrame)));
}

void Engine::Draw() {
  uint32_t imageIndex = 0;

  /*--------------------*/
  //     resize        //
  /*------------------*/

  if (camera->activeWindow) {

    // resize if resized
    this->HandleResize();

    //---------------------//
    //---COMPUTE QUEUE-----//
    //---------------------//

    //// wait for fences
    this->ComputeFence(currentFrame);

    ///* record compute commands section */
    //// compute command buffer vector
    std::vector<VkCommandBuffer> computeCommandBuffers;
    // computeCommandBuffers =
    // this->renderers.mainRenderer.RecordCompute(currentFrame);
    computeCommandBuffers =
        this->renderers.defaultRenderer->ComputeCommands(currentFrame);
    // compute pipeline wait stages
    std::vector<VkPipelineStageFlags> computeWaitStages = {
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

    //  compute submit info
    std::vector<VkSemaphore> computeSemaphores = {
        this->ComputeSemaphore(currentFrame)};

    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.pWaitDstStageMask = computeWaitStages.data();
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.commandBufferCount =
        static_cast<uint32_t>(computeCommandBuffers.size());
    computeSubmitInfo.pCommandBuffers = computeCommandBuffers.data();
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores =
        &this->sync.computeFinishedSemaphore[currentFrame];

    // submit compute queue
    validate_vk_result(vkQueueSubmit(pEngineCore->queue.compute, 1,
                                     &computeSubmitInfo,
                                     sync.computeFences[currentFrame]));

    //------------------------//
    //-----GRAPHICS QUEUE----//
    //----------------------//
    //
    // wait for draw fences
    validate_vk_result(vkWaitForFences(this->LogicalDevice(), 1,
                                       &sync.drawFences[currentFrame], VK_TRUE,
                                       std::numeric_limits<uint64_t>::max()));

    validate_vk_result(vkResetFences(this->LogicalDevice(), 1,
                                     &sync.drawFences[currentFrame]));

    // acquire next image
    VkResult acquireImageResult = vkAcquireNextImageKHR(
        this->LogicalDevice(), swapchainData.swapchainKHR, UINT64_MAX,
        sync.presentFinishedSemaphore[currentFrame], VK_NULL_HANDLE,
        &imageIndex);

    // return if something changed // handle swapchain and framebuffer resize
    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR ||
        acquireImageResult == VK_SUBOPTIMAL_KHR) {
      return;
    }

    /*------------------------------------*/
    //     update uniform buffers        //
    /*----------------------------------*/

    // Update model class animation
    // iterate through model list

    bool isBLASUpdated = false;

    this->renderers.defaultRenderer->UpdateShaderBuffers(
        deltaTime, timer, this->UI.rendererData.lightPosition);

    for (int i = 0; i < this->UI.modelData.updateBLAS.size(); i++) {
      this->renderers.defaultRenderer->pModelData()->updateBLAS[i] = false;
    }

    /*----------------------------------*/
    //     record command buffer       //
    /*--------------------------------*/

    // begin graphics command buffer
    this->BeginGraphicsCommandBuffer(currentFrame);

    this->renderers.defaultRenderer->GraphicsCommands(
        currentFrame, this->UI.rendererData.showIDImage);

    /*---------------------------------*/
    //     render dearImGui           //
    /*-------------------------------*/

    this->HandleUI();

    /*------------------------------------*/
    //    end record command buffer      //
    /*----------------------------------*/

    this->EndGraphicsCommandBuffer(currentFrame);

    /*-----------------------*/
    //    queue submit      //
    /*---------------------*/

    // pipeline wait stages
    std::vector<VkPipelineStageFlags> waitStages = {
        VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    std::vector<VkSemaphore> graphicsWaitSemaphores = {
        this->ComputeSemaphore(currentFrame),
        this->PresentSemaphore(currentFrame)};

    std::vector<VkSemaphore> graphicsSignalSemaphores = {
        this->RenderFinishedSemaphore(currentFrame)};

    std::vector<VkCommandBuffer> graphicsCommandBuffer = {
        this->GraphicsCommandBuffer(currentFrame)};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.waitSemaphoreCount =
        static_cast<uint32_t>(graphicsWaitSemaphores.size());
    submitInfo.pWaitSemaphores = graphicsWaitSemaphores.data();
    submitInfo.signalSemaphoreCount =
        static_cast<uint32_t>(graphicsSignalSemaphores.size());
    submitInfo.pSignalSemaphores = graphicsSignalSemaphores.data();
    submitInfo.commandBufferCount =
        static_cast<uint32_t>(graphicsCommandBuffer.size());
    submitInfo.pCommandBuffers = graphicsCommandBuffer.data();

    // Submit command buffers to the graphics queue
    validate_vk_result(vkQueueSubmit(queue.graphics, 1, &submitInfo,
                                     sync.drawFences[currentFrame]));

    // Prepare present info for swapchain presentation
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &sync.renderFinishedSemaphore[currentFrame];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &sync.presentFinishedSemaphore[currentFrame];
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchainData.swapchainKHR;
    presentInfo.pImageIndices = &imageIndex;

    // Present the rendered image to the screen
    validate_vk_result(vkQueuePresentKHR(queue.graphics, &presentInfo));

    // Update current frame index
    this->currentFrame = (this->currentFrame + 1) % frame_draws;
  }
}
} // namespace gtp
