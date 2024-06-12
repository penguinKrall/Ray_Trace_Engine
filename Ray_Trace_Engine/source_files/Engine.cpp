#include "Engine.hpp"

namespace gtp {

	// -- ctor
	Engine Engine::engine() {
		return Engine();
	}

	// -- init
	VkResult Engine::InitEngine() {


		//init core
		InitCore();

		lastX = float(this->width) / 2.0f;
		lastY = float(this->height) / 2.0f;

		posX = float(this->width) / 2.0f;
		posY = float(this->height) / 2.0f;

		//init ui
		InitUI();

		//init renderers
		InitRenderers();

		return VK_SUCCESS;

	}

	// -- Run
	void Engine::Run() {

		//main loop
		while (!glfwWindowShouldClose(windowGLFW)) {


			//poll events
			glfwPollEvents();

			//timer
			auto now = static_cast<float>(glfwGetTime());
			this->deltaTime = now - this->lastTime;
			this->lastTime = now;

			//std::cout << " delta time: " << deltaTime << std::endl;

			//handle input
			userInput();

			//draw
			Draw();

			//set to false - wont update buffers again unless view changes
			pEngineCore->camera->viewUpdated = false;

			this->timer += deltaTime;
		}

	}

	// -- terminate
	//@note destroys objects created related to renderers and core
	void Engine::Terminate() {

		//wait idle before destroy
		vkDeviceWaitIdle(devices.logical);

		//ui
		UI.DestroyUI();

		//renderers
		//gltf_viewer
		this->renderers.gltfViewer.Destroy_gltf_viewer();

		// build scene
		//this->renderers.gltfViewer.Destroy_Build_Scene();

		////multi blas
		//this->renderers.multiBlas.Destroy_multi_blas();
		//
		////complex scene
		//this->renderers.complexScene.Destroy_Complex_Scene();
		//
		////multi model
		//this->renderers.multiModel.Destroy_Multi_Model();
		//
		////gltf reflections
		//this->renderers.gltfReflections.DestroyglTF_Reflections();
		//
		////math compute
		//this->renderers.mathCompute.Destroy_Math_Compute();
		//
		////rt animation
		//this->renderers.gltfAnimation.DestroyglTFAnimation();
		//
		////glTFTextured
		//this->renderers.gltfTextured.DestroyglTFTextured();
		//
		////shadow
		//this->renderers.gltfShadows.DestroyglTFShadows();
		//
		////rt basic/texture
		//this->renderers.raytracing.DestroyRaytracing();


		//core
		DestroyCore();

	}

	void Engine::userInput() {

		//float deltaTime = deltaTime;

		glfwGetCursorPos(windowGLFW, &posX, &posY);

		// Print mouse coordinates (you can use them as needed)
		//printf("Mouse Coordinates: %.2f, %.2f\n", posX, posY);

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

		//std::cout << "\ninitializing raytracing class - quad with grate texture\n''''''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.raytracing = Raytracing(this->pEngineCore);
		//std::cout << "\nfinished initializing raytracing class\n'''''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing glTFShadows class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.gltfShadows = glTFShadows(this->pEngineCore);
		//std::cout << "\nfinished initializing glTFShadows class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing glTFTextured class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.gltfTextured = glTFTextured(this->pEngineCore);
		//std::cout << "\nfinished initializing glTFTextured class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing glTFAnimation class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.gltfAnimation = glTFAnimation(this->pEngineCore);
		//std::cout << "\nfinished initializing glTFAnimation class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing glTF_PBR class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.gltfPBR = glTF_PBR(this->pEngineCore);
		//std::cout << "\nfinished initializing glTF_PBR class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing Math_Compute class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.mathCompute = Math_Compute(this->pEngineCore);
		//std::cout << "\nfinished initializing Math_Compute class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing glTF_Reflections class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.gltfReflections = glTF_Reflections(this->pEngineCore);
		//std::cout << "\nfinished initializing glTF_Reflections class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing Multi_Model class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.multiModel = Multi_Model(this->pEngineCore);
		//std::cout << "\nfinished initializing Multi_Model class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing Complex_Scene class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.complexScene = Complex_Scene(this->pEngineCore);
		//std::cout << "\nfinished initializing Complex_Scene class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//
		//std::cout << "\ninitializing multi_blas class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.multiBlas = multi_blas(this->pEngineCore);
		//std::cout << "\nfinished initializing multi_blas class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;

		//std::cout << "\ninitializing Build_Scene class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		//this->renderers.gltfViewer = Build_Scene(this->pEngineCore);
		//std::cout << "\nfinished initializing Build_Scene class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;

		std::cout << "\ninitializing gltf_viewer class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;
		this->renderers.gltfViewer = gltf_viewer(this->pEngineCore);
		std::cout << "\nfinished initializing gltf_viewer class\n'''''''''''''''''''''''''''''''''''''''''''''''\n" << std::endl;

	}

	void Engine::InitUI() {
		UI = CoreUI(this->pEngineCore);
		std::cout << "\nfinished initializing UI\n''''''''''''''''''''''''\n" << std::endl;
	}

	void Engine::BeginGraphicsCommandBuffer(int currentFrame) {

		//command buffer begin info
		VkCommandBufferBeginInfo cmdBufInfo{};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		//begin command buffer
		if (vkBeginCommandBuffer(pEngineCore->commandBuffers.graphics[currentFrame], &cmdBufInfo) != VK_SUCCESS) {
			throw std::invalid_argument("failed to begin recording graphics command buffer");
		}

	}

	void Engine::EndGraphicsCommandBuffer(int currentFrame) {

		if (vkEndCommandBuffer(commandBuffers.graphics[currentFrame]) != VK_SUCCESS) {
			throw std::invalid_argument("failed to end recording command buffer");
		}

	}

	void Engine::Draw() {

		uint32_t imageIndex = 0;

		/*--------------------*/
		//     resize        //
		/*------------------*/

		if (camera->framebufferResized) {
			//wait idle
			vkDeviceWaitIdle(devices.logical);
			//recreate semaphores, fences
			RecreateSyncObjects();
			//recreate swapchain, swapchain images/views
			RecreateCoreSwapchain();
			//recreate god knows what
			renderers.gltfViewer.HandleResize();
			//recreate ui framebuffers
			UI.RecreateFramebuffers();
			//Flaggy McFlaggerson
			camera->framebufferResized = false;
			//ret
			return;
		}

		/*---------------------------------------*/
		//     wait stages / draw fences        //
		/*-------------------------------------*/

		//---------------------//
		//---COMPUTE QUEUE-----//
		//---------------------//

		//if (camera->activeWindow) {


			//wait for fences  
		vkWaitForFences(devices.logical, 1, &sync.computeFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		//reset fences
		vkResetFences(devices.logical, 1, &sync.computeFences[currentFrame]);

		VkCommandBufferBeginInfo computeBeginInfo{};
		computeBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		// -- multi blas compute
		vkResetCommandBuffer(this->renderers.gltfViewer.gltfCompute.commandBuffers[currentFrame],
			/*VkCommandBufferResetFlagBits*/ 0);

		//begin compute command buffer
		if (vkBeginCommandBuffer(this->renderers.gltfViewer.gltfCompute.commandBuffers[currentFrame], &computeBeginInfo)
			!= VK_SUCCESS) {
			throw std::invalid_argument("failed to begin recording compute command buffer!");
		}

		//record compute commands
		this->renderers.gltfViewer.gltfCompute.RecordComputeCommands(currentFrame);

		//end compute command buffer
		if (vkEndCommandBuffer(this->renderers.gltfViewer.gltfCompute.commandBuffers[currentFrame])
			!= VK_SUCCESS) {
			throw std::invalid_argument("failed to record compute command buffer!");
		}
		//compute command buffer vector
		std::vector<VkCommandBuffer> computeCommands = {
			this->renderers.gltfViewer.gltfCompute.commandBuffers[currentFrame]
		};

		//compute pipeline wait stages
		std::vector<VkPipelineStageFlags> computeWaitStages = {
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT
		};

		//compute submit info
		VkSubmitInfo computeSubmitInfo{};
		computeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		computeSubmitInfo.pWaitDstStageMask = computeWaitStages.data();
		computeSubmitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.commandBufferCount = static_cast<uint32_t>(computeCommands.size());
		computeSubmitInfo.pCommandBuffers = computeCommands.data();
		computeSubmitInfo.signalSemaphoreCount = 1;
		computeSubmitInfo.pSignalSemaphores = &this->sync.computeFinishedSemaphore[currentFrame];

		//submit compute queue
		if (vkQueueSubmit(pEngineCore->queue.compute,
			1, &computeSubmitInfo, sync.computeFences[currentFrame]) != VK_SUCCESS) {
			throw std::invalid_argument("failed to submit compute command buffer!");
		}

		//------------------------//
		//-----GRAPHICS QUEUE----//
		//----------------------//

		//vkDeviceWaitIdle(this->pEngineCore->devices.logical);

		//wait for draw fences
		vkWaitForFences(devices.logical, 1, &sync.drawFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		vkResetFences(devices.logical, 1, &sync.drawFences[currentFrame]);

		//acquire next image
		VkResult acquireImageResult = vkAcquireNextImageKHR(devices.logical, swapchainData.swapchainKHR,
			UINT64_MAX, sync.presentFinishedSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

		//return if something changed // handle swapchain and framebuffer resize
		if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR || acquireImageResult == VK_SUBOPTIMAL_KHR) {
			return;
		}

		/*------------------------------------*/
		//     update uniform buffers        //
		/*----------------------------------*/

		//update model class animation
		this->renderers.gltfViewer.assets.animatedModel->updateAnimation(2, deltaTime);

		//update compute class joint buffer
		this->renderers.gltfViewer.gltfCompute.UpdateJointBuffer();

		this->renderers.gltfViewer.UpdateUniformBuffer(timer);
		this->renderers.gltfViewer.UpdateBLAS();
		this->renderers.gltfViewer.UpdateTLAS();

		/*----------------------------------*/
		//     record command buffer       //
		/*--------------------------------*/

		BeginGraphicsCommandBuffer(currentFrame);

		// multi blas
		this->renderers.gltfViewer.RebuildCommandBuffers(currentFrame);

		/*---------------------------------*/
		//     render dearImGui           //
		/*-------------------------------*/

		//update UI input
		this->UI.Input();

		//update UI vertex/index buffers
		this->UI.update(currentFrame);

		//draw UI
		this->UI.DrawUI(commandBuffers.graphics[currentFrame], currentFrame);

		/*------------------------------------*/
		//    end record command buffer      //
		/*----------------------------------*/

		this->EndGraphicsCommandBuffer(currentFrame);

		/*-----------------------*/
		//    queue submit      //
		/*---------------------*/

			//pipeline wait stages
		std::vector<VkPipelineStageFlags> waitStages = {
				VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
				VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
		};

		std::vector<VkSemaphore> graphicsSemaphores = {
			sync.computeFinishedSemaphore[currentFrame],
			sync.presentFinishedSemaphore[currentFrame]
		};

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitDstStageMask = waitStages.data();
		submitInfo.waitSemaphoreCount = static_cast<uint32_t>(graphicsSemaphores.size());
		submitInfo.pWaitSemaphores = graphicsSemaphores.data();
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &sync.renderFinishedSemaphore[currentFrame];
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers.graphics[currentFrame];

		// Submit command buffers to the graphics queue
		if (vkQueueSubmit(queue.graphics, 1, &submitInfo, sync.drawFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("Failed to submit command buffer to graphics queue!");
		}

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
		VkResult presentImageResult = vkQueuePresentKHR(queue.graphics, &presentInfo);

		// Update current frame index
		this->currentFrame = (this->currentFrame + 1) % frame_draws;

	}

}