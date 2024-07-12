#pragma once

#include <EngineCore.hpp>
#include <CoreUI.hpp>

namespace gtp {
  // -- class to handle all the loading screen images/buffers/draws
class LoadingScreen {
private:


  /* private variables */
  
  //background texture
  gtp::Utilities_EngCore::ImageData bgTexture{};
  VkRenderPass renderPass;

  //core pointer
  EngineCore* pEngineCore = nullptr;

  /* private functions */
  // -- init class function
  void InitLoadingScreen(EngineCore* engineCorePtr);

  // -- create loading screen image
  void CreateLoadingScreenImage(const std::string& fileName);

  // -- copy loading screen image to swapchain images
  void CopyToSwapchainImage(VkImageLayout srcLayout, VkImageLayout dstLayout);

  // -- draw loading screen
  void DrawLoadingScreen(CoreUI* uiPtr, std::string loadingMessage);


public:

  /* public variables*/
  //none

  /* public functions */

  //default ctor
  LoadingScreen();

  //init ctor
  LoadingScreen(EngineCore* engineCorePtr);

  //update
  void UpdateLoadingScreen();

  //draw
  void Draw(CoreUI* uiPtr, std::string loadingMessage);

  void DestroyLoadingScreen();
};
} // namespace gtp
