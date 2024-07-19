#include "Engine.hpp"

namespace gtp {

// -- ctor
Engine Engine::engine() { return Engine(); }

// -- init
VkResult Engine::InitEngine() {
  // init core
  this->InitCore();

  // init xy pos
  this->InitXYPos();

  // init ui
  this->InitUI();

  // poll events
  glfwPollEvents();
  this->UpdateDeltaTime();
  this->userInput();

  // initialize loading screen
  this->loadingScreen = gtp::LoadingScreen(this->pEngineCore);

  // draw loading screen
  if (!glfwWindowShouldClose(windowGLFW)) {
    this->loadingScreen.Draw(&this->UI, "Loading Renderer...");
  }

  // init renderers
  this->InitRenderers();

  // update loading screen
  if (!glfwWindowShouldClose(windowGLFW)) {
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

  return VK_SUCCESS;
}

void Engine::InitXYPos() {
  lastX = float(this->width) / 2.0f;
  lastY = float(this->height) / 2.0f;

  posX = float(this->width) / 2.0f;
  posY = float(this->height) / 2.0f;
}

// -- Run
void Engine::Run() {
  // main loop
  while (!glfwWindowShouldClose(windowGLFW)) {
    // poll events
    glfwPollEvents();

    // timer
    this->UpdateDeltaTime();

    if (camera->activeWindow) {
      // handle input
      this->userInput();

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
    }

    // set to false - wont update buffers again unless view changes
    pEngineCore->camera->viewUpdated = false;

    // Update Animation Timer
    this->UpdateAnimationTimer();
  }
}

// -- terminate
//@note destroys objects created related to renderers and core
void Engine::Terminate() {
  // wait idle before destroy
  vkDeviceWaitIdle(devices.logical);

  // loading screen
  this->loadingScreen.DestroyLoadingScreen();

  // ui
  UI.DestroyUI();

  // renderers
  // MainRenderer
  this->renderers.defaultRenderer->Destroy();
  this->renderers.mainRenderer.Destroy();

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
    this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
    this->renderers.mainRenderer.SetModelData(&this->UI.modelData);
    this->isUIUpdated = false;
  }
}

void Engine::HandleUI() {
  // update UI input
  this->UI.Input(&this->UI.modelData);

  // update UI vertex/index buffers
  this->UI.UpdateBuffers();

  // draw UI
  this->UI.DrawUI(commandBuffers.graphics[currentFrame], currentFrame);
}

void Engine::RetrieveColorID() {
  if (this->pEngineCore->camera->mouseOnWindow) {
    if (this->isLMBPressed) {
      this->renderers.mainRenderer.GetTools()
          .objectMouseSelect.RetrieveObjectID();
      this->isLMBPressed = false;
    }
  }
}

void Engine::LoadModel() {
  // continue if ui load model is true
  if (this->UI.modelData.loadModel) {
    // set main renderer model data to ui model data
    this->renderers.mainRenderer.SetModelData(&this->UI.modelData);

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
    this->renderers.mainRenderer.HandleLoadModel(loadingFlags);

    this->UI.loadModelFlags.flipY = false;
    this->UI.loadModelFlags.preMultiplyColors = false;
    this->UI.loadModelFlags.preTransform = false;
    this->UI.loadModelFlags.loadModelName = "none";

    // set ui model data to main renderer model data
    this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
  }
}

void Engine::DeleteModel() {
  if (this->UI.modelData.deleteModel) {
    this->renderers.mainRenderer.SetModelData(&this->UI.modelData);
    this->renderers.mainRenderer.DeleteModel();

    // this->renderers.mainRenderer.assets.modelData.deleteModel = false;
    this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
    this->UI.modelData.isUpdated = true;
  }
}

void Engine::UpdateRenderer() {
  if (this->UI.modelData.isUpdated) {
    // -- update renderer model data struct with ui
    this->renderers.mainRenderer.SetModelData(&this->UI.modelData);
    // this->renderers.mainRenderer.assets.modelData.isUpdated = false;
    //  -- update ui data to updated renderer model data with updated flags
    this->UI.SetModelData(this->renderers.mainRenderer.GetModelData());
  }
}

void Engine::HandleResize() {
  if (camera->activeWindow) {
    if (camera->framebufferResized) {
      // wait idle
      vkDeviceWaitIdle(devices.logical);
      // recreate semaphores, fences
      RecreateSyncObjects();
      // recreate swapchain, swapchain images/views
      RecreateCoreSwapchain();
      // recreate god knows what
      renderers.mainRenderer.HandleResize();
      // default renderer resize
      this->renderers.defaultRenderer->Resize();
      // recreate ui framebuffers
      UI.RecreateFramebuffers();
      // Flaggy McFlaggerson
      camera->framebufferResized = false;
      // ret
      return;
    }
  }
}

void Engine::userInput() {
  // float deltaTime = deltaTime;

  glfwGetCursorPos(windowGLFW, &posX, &posY);

  // Print mouse coordinates (you can use them as needed)
  // printf("Mouse Coordinates: %.2f, %.2f\n", posX, posY);

  auto xoffset = static_cast<float>(posX - lastX);
  auto yoffset = static_cast<float>(lastY - posY);

  lastX = posX;
  lastY = posY;

  if (glfwGetMouseButton(windowGLFW, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_TRUE) {
    camera->viewUpdated = true;
    camera->ProcessMouseMovement(xoffset, yoffset);
  }

  if (glfwGetMouseButton(windowGLFW, GLFW_MOUSE_BUTTON_LEFT) == GLFW_TRUE) {
    this->isLMBPressed = true;
  }

  // if (glfwGetKey(windowGLFW, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
  //   camera->viewUpdated = true;
  //   glfwSetWindowShouldClose(windowGLFW, true);
  // }

  if (glfwGetKey(windowGLFW, GLFW_KEY_W) == GLFW_PRESS) {
    camera->viewUpdated = true;
    camera->ProcessKeyboard(Movement::FORWARD, deltaTime);
  }

  if (glfwGetKey(windowGLFW, GLFW_KEY_S) == GLFW_PRESS) {
    camera->viewUpdated = true;
    camera->ProcessKeyboard(Movement::BACKWARD, deltaTime);
  }

  if (glfwGetKey(windowGLFW, GLFW_KEY_A) == GLFW_PRESS) {
    camera->viewUpdated = true;
    camera->ProcessKeyboard(Movement::LEFT, deltaTime);
  }

  if (glfwGetKey(windowGLFW, GLFW_KEY_D) == GLFW_PRESS) {
    camera->viewUpdated = true;
    camera->ProcessKeyboard(Movement::RIGHT, deltaTime);
  }

  if (glfwGetKey(windowGLFW, GLFW_KEY_UP) == GLFW_TRUE) {
    camera->viewUpdated = true;
    camera->ProcessKeyboard(Movement::UP, deltaTime);
  }

  if (glfwGetKey(windowGLFW, GLFW_KEY_DOWN) == GLFW_TRUE) {
    camera->viewUpdated = true;
    camera->ProcessKeyboard(Movement::DOWN, deltaTime);
  }
}

void Engine::InitRenderers() {
  std::cout << "\ninitializing MainRenderer "
               "class\n'''''''''''''''''''''''''''''''''''''''''''''''\n"
            << std::endl;

  // output to loading screen
  this->loadingScreen.Draw(&this->UI, "Loading Main 'Renderer'");

  this->renderers.mainRenderer = MainRenderer(this->pEngineCore);
  std::cout << "\nfinished initializing MainRenderer "
               "class\n'''''''''''''''''''''''''''''''''''''''''''''''\n"
            << std::endl;

  std::cout << "\ninitializing RenderBase "
               "class\n'''''''''''''''''''''''''''''''''''''''''''''''\n"
            << std::endl;

  // outtput to loading screen
  this->loadingScreen.Draw(&this->UI, "Loading Default Renderer");

  this->renderers.defaultRenderer = new DefaultRenderer(this->pEngineCore);
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
      pEngineCore->commandBuffers.graphics[currentFrame], &cmdBufInfo));
}

void Engine::EndGraphicsCommandBuffer(int currentFrame) {
  validate_vk_result(vkEndCommandBuffer(commandBuffers.graphics[currentFrame]));
}

void Engine::Draw() {
  uint32_t imageIndex = 0;

  /*--------------------*/
  //     resize        //
  /*------------------*/

  if (camera->activeWindow) {
    if (camera->framebufferResized) {
      // wait idle
      vkDeviceWaitIdle(devices.logical);
      // recreate semaphores, fences
      RecreateSyncObjects();
      // recreate swapchain, swapchain images/views
      RecreateCoreSwapchain();
      // recreate god knows what
      renderers.mainRenderer.HandleResize();
      // recreate ui framebuffers
      UI.RecreateFramebuffers();
      // Flaggy McFlaggerson
      camera->framebufferResized = false;
      // ret
      return;
    }

    //---------------------//
    //---COMPUTE QUEUE-----//
    //---------------------//

    // wait for fences
    if (vkWaitForFences(devices.logical, 1, &sync.computeFences[currentFrame],
                        VK_TRUE,
                        std::numeric_limits<uint64_t>::max()) != VK_SUCCESS) {
      throw std::invalid_argument("failed to wait for compute fences");
    }

    // reset fences
    vkResetFences(devices.logical, 1, &sync.computeFences[currentFrame]);

    VkCommandBufferBeginInfo computeBeginInfo{};
    computeBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    /* record compute commands section */
    // compute command buffer vector
    std::vector<VkCommandBuffer> computeCommands;

    // iterate through renderer compute instances
    for (int i = 0;
         i < this->renderers.mainRenderer.GetComputeInstances().size(); i++) {
      // if one element is initialized...
      //   begin process of reset/begin/record commands from that one
      //   instance/submit
      if (this->renderers.mainRenderer.GetComputeInstances()[i] != nullptr) {
        validate_vk_result(vkResetCommandBuffer(
            this->renderers.mainRenderer.GetComputeInstances()[i]
                ->commandBuffers[currentFrame],
            /*VkCommandBufferResetFlagBits*/ 0));

        //  begin compute command buffer
        validate_vk_result(vkBeginCommandBuffer(
            this->renderers.mainRenderer.GetComputeInstances()[i]
                ->commandBuffers[currentFrame],
            &computeBeginInfo));

        //  record compute commands
        // check that the model is one with animations and selected to be
        // animated

        if (this->renderers.mainRenderer.GetModelData()
                ->animatedModelIndex[i] == 1) {
          if (this->renderers.mainRenderer.GetModelData()->isAnimated[i]) {
            // verify current model is being animated
            this->renderers.mainRenderer.GetComputeInstances()[i]
                ->RecordComputeCommands(currentFrame);
          }
        }

        //  end compute command buffer
        validate_vk_result(vkEndCommandBuffer(
            this->renderers.mainRenderer.GetComputeInstances()[i]
                ->commandBuffers[currentFrame]));
        computeCommands.push_back(
            this->renderers.mainRenderer.GetComputeInstances()[i]
                ->commandBuffers[currentFrame]);
      }
    }

    // update compute commands to be waited on
    computeCommands =
        this->renderers.mainRenderer.RecordParticleComputeCommands(
            currentFrame, computeCommands);

    // compute pipeline wait stages
    std::vector<VkPipelineStageFlags> computeWaitStages = {
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};
    //  compute submit info
    VkSubmitInfo computeSubmitInfo{};
    computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    computeSubmitInfo.pWaitDstStageMask = computeWaitStages.data();
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.commandBufferCount =
        static_cast<uint32_t>(computeCommands.size());
    computeSubmitInfo.pCommandBuffers = computeCommands.data();
    computeSubmitInfo.signalSemaphoreCount = 1;
    computeSubmitInfo.pSignalSemaphores =
        &this->sync.computeFinishedSemaphore[currentFrame];

    // submit compute queue
    validate_vk_result(vkQueueSubmit(pEngineCore->queue.compute, 1,
                                     &computeSubmitInfo,
                                     sync.computeFences[currentFrame]));

    // vkDeviceWaitIdle(this->pEngineCore->devices.logical);

    //------------------------//
    //-----GRAPHICS QUEUE----//
    //----------------------//

    // vkDeviceWaitIdle(this->pEngineCore->devices.logical);

    // wait for draw fences
    validate_vk_result(vkWaitForFences(devices.logical, 1,
                                       &sync.drawFences[currentFrame], VK_TRUE,
                                       std::numeric_limits<uint64_t>::max()));

    validate_vk_result(
        vkResetFences(devices.logical, 1, &sync.drawFences[currentFrame]));

    // acquire next image
    VkResult acquireImageResult = vkAcquireNextImageKHR(
        devices.logical, swapchainData.swapchainKHR, UINT64_MAX,
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

    this->renderers.mainRenderer.UpdateAnimations(deltaTime);

    this->renderers.mainRenderer.UpdateUniformBuffer(
        timer, this->UI.rendererData.lightPosition);

    this->renderers.mainRenderer.HandleAccelerationStructureUpdate();

    // this->renderers.mainRenderer.UpdateBLAS();
    //
    // this->renderers.mainRenderer.UpdateTLAS();

    for (int i = 0; i < this->UI.modelData.updateBLAS.size(); i++) {
      this->UI.modelData.updateBLAS[i] = false;
    }

    /*----------------------------------*/
    //     record command buffer       //
    /*--------------------------------*/

    BeginGraphicsCommandBuffer(currentFrame);

    // multi blas
    this->renderers.mainRenderer.RebuildCommandBuffers(
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

    std::vector<VkSemaphore> graphicsSemaphores = {
        sync.computeFinishedSemaphore[currentFrame],
        sync.presentFinishedSemaphore[currentFrame]};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask = waitStages.data();
    submitInfo.waitSemaphoreCount =
        static_cast<uint32_t>(graphicsSemaphores.size());
    submitInfo.pWaitSemaphores = graphicsSemaphores.data();
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &sync.renderFinishedSemaphore[currentFrame];
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers.graphics[currentFrame];

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
