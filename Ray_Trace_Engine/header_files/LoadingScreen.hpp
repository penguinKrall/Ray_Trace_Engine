#pragma once

#include <EngineCore.hpp>
#include <CoreUI.hpp>

namespace gtp {
  // -- class to handle all the loading screen images/buffers/draws
class LoadingScreen {
private:
  /* private variables */
  VkRenderPass renderPass;

  //core pointer
  EngineCore* pEngineCore = nullptr;

  /* private functions */
  // -- init class function
  void InitLoadingScreen(EngineCore* engineCorePtr);

  // -- create loading screen image
  void CreateLoadingScreenImage(const std::string& fileName);

  // -- draw loading screen
  void DrawLoadingScreen(CoreUI* uiPtr);


public:

  /* public variables*/
  //none

  /* public functions */

  //default ctor
  LoadingScreen();

  //init ctor
  LoadingScreen(EngineCore* engineCorePtr);

  //draw
  void Draw(CoreUI* uiPtr);

};
} // namespace gtp
