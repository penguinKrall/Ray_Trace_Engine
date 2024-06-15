#include "Engine.hpp"

namespace gtp {

// -- ctor
Engine Engine::engine() { return Engine(); }

// -- init
VkResult Engine::InitEngine() {

  // init core
  InitCore();

  lastX = float(this->width) / 2.0f;
  lastY = float(this->height) / 2.0f;

  posX = float(this->width) / 2.0f;
  posY = float(this->height) / 2.0f;

  // init ui
  InitUI();

  // init renderers
  InitRenderers();

  return VK_SUCCESS;
}

// -- Run
void Engine::Run() {

  // main loop
  while (!glfwWindowShouldClose(windowGLFW)) {

    // poll events
    glfwPollEvents();

    // timer
    auto now = static_cast<float>(glfwGetTime());
    this->deltaTime = now - this->lastTime;
    this->lastTime = now;

    // std::cout << " delta time: " << deltaTime << std::endl;

    // handle input
    userInput();

    // update ui
    this->UpdateUI();

    // if (this->initialUpdate) {
    //   this->renderers.mainRenderer.UpdateUIData(&this->UI.modelData);
    //   this->initialUpdate = false;
    //   this->UI.modelData.isUpdated = false;
    // }

    // draw
    Draw();

    if (this->UI.modelData.isUpdated) {
      this->renderers.mainRenderer.UpdateModelTransforms(&this->UI.modelData);
      this->UI.SetModelData(&this->renderers.mainRenderer.assets.modelData);
      this->UI.modelData.isUpdated = false;
    }

    // set to false - wont update buffers again unless view changes
    pEngineCore->camera->viewUpdated = false;

    this->timer += deltaTime;
  }
}

// -- terminate
//@note destroys objects created related to renderers and core
void Engine::Terminate() {

  // wait idle before destroy
  vkDeviceWaitIdle(devices.logical);

  // ui
  UI.DestroyUI();

  // renderers
  // MainRenderer
  this->renderers.mainRenderer.Destroy_MainRenderer();

  // core
  DestroyCore();
}

void Engine::UpdateUI() {
  if (this->isUIUpdated) {
    this->renderers.mainRenderer.UpdateUIData(
        &this->renderers.mainRenderer.assets.modelData);
    this->UI.SetModelData(&this->renderers.mainRenderer.assets.modelData);
    this->isUIUpdated = false;
  }
}

void Engine::HandleUI() {
  // update UI input
  this->UI.Input(&this->UI.modelData);

  // update UI vertex/index buffers
  this->UI.UpdateBuffers(currentFrame);

  // draw UI
  this->UI.DrawUI(commandBuffers.graphics[currentFrame], currentFrame);
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

  if (glfwGetKey(windowGLFW, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    camera->viewUpdated = true;
    glfwSetWindowShouldClose(windowGLFW, true);
  }

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
  this->renderers.mainRenderer = MainRenderer(this->pEngineCore);
  std::cout << "\nfinished initializing MainRenderer "
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

  /*---------------------------------------*/
  //     wait stages / draw fences        //
  /*-------------------------------------*/

  //---------------------//
  //---COMPUTE QUEUE-----//
  //---------------------//

  // if (camera->activeWindow) {

  // wait for fences
  validate_vk_result(vkWaitForFences(devices.logical, 1,
                                     &sync.computeFences[currentFrame], VK_TRUE,
                                     std::numeric_limits<uint64_t>::max()));

  // reset fences
  vkResetFences(devices.logical, 1, &sync.computeFences[currentFrame]);

  VkCommandBufferBeginInfo computeBeginInfo{};
  computeBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  // -- multi blas compute
  validate_vk_result(vkResetCommandBuffer(
      this->renderers.mainRenderer.gltfCompute.commandBuffers[currentFrame],
      /*VkCommandBufferResetFlagBits*/ 0));

  // begin compute command buffer
  validate_vk_result(vkBeginCommandBuffer(
      this->renderers.mainRenderer.gltfCompute.commandBuffers[currentFrame],
      &computeBeginInfo));

  // record compute commands
  this->renderers.mainRenderer.gltfCompute.RecordComputeCommands(currentFrame);

  // end compute command buffer
  validate_vk_result(vkEndCommandBuffer(
      this->renderers.mainRenderer.gltfCompute.commandBuffers[currentFrame]));

  // compute command buffer vector
  std::vector<VkCommandBuffer> computeCommands = {
      this->renderers.mainRenderer.gltfCompute.commandBuffers[currentFrame]};

  // compute pipeline wait stages
  std::vector<VkPipelineStageFlags> computeWaitStages = {
      VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT};

  // compute submit info
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
      sync.presentFinishedSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

  // return if something changed // handle swapchain and framebuffer resize
  if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR ||
      acquireImageResult == VK_SUBOPTIMAL_KHR) {
    return;
  }

  /*------------------------------------*/
  //     update uniform buffers        //
  /*----------------------------------*/

  // update model class animation
  this->renderers.mainRenderer.assets.models[0]->updateAnimation(0, deltaTime);

  // update compute class joint buffer
  this->renderers.mainRenderer.gltfCompute.UpdateJointBuffer();

  this->renderers.mainRenderer.UpdateUniformBuffer(timer);
  this->renderers.mainRenderer.UpdateBLAS();
  this->renderers.mainRenderer.UpdateTLAS();

  /*----------------------------------*/
  //     record command buffer       //
  /*--------------------------------*/

  BeginGraphicsCommandBuffer(currentFrame);

  // multi blas
  this->renderers.mainRenderer.RebuildCommandBuffers(currentFrame);

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
      VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
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

} // namespace gtp
