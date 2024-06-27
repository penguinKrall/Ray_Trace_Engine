#pragma once

#include <CoreUI.hpp>
#include <EngineCore.hpp>
#include <MainRenderer.hpp>

namespace gtp {

// -- engine
//@brief contains core objects and renderers
class Engine : private EngineCore {
private:
  float deltaTime = 0.0f;
  float lastTime = 0.0f;
  float timer = 0.0f;

  // double lastX = 0.0f;
  // double lastY = 0.0f;
  //
  // double posX = 0.0f;
  // double posY = 0.0f;

  // -- user input
  void userInput();
  
  //mouse clicked on window
  bool isLMBPressed = false;

  // -- renderers
  struct Renderers {
    MainRenderer mainRenderer;
  };

  Renderers renderers{};

  // UI
  CoreUI UI{};

  // -- init renderers
  //@brief passes core instance pointer to renderer/s
  void InitRenderers();

  // -- init ui
  //@brief passes core instance pointer to UI
  void InitUI();

  // -- begin graphics command buffer
  //@brief begins graphics command buffer
  void BeginGraphicsCommandBuffer(int currentFrame);

  // -- end graphics command buffer
  //@brief ends graphics command buffer
  void EndGraphicsCommandBuffer(int currentFrame);

  // -- ui update flag
  //@brief set to true to initialize ui data
  bool initialUpdate = true;
  bool isUIUpdated = true;

public:
  // -- engine constructor
  Engine engine();

  // -- init engine
  //@brief calls inherited 'initCore' func and initializes renderers
  VkResult InitEngine();

  // -- init X and Y pos
  void InitXYPos();

  // -- draw
  //@brief handles update ubos, record command buffers, queue submit, and
  // present submit
  void Draw();

  // -- run
  //@brief main render loop
  void Run();

  // -- terminate
  //@brief destroy. everything.
  void Terminate();

  // -- update delta time
  void UpdateDeltaTime();

  // -- update animation timer
  void UpdateAnimationTimer();

  // -- update ui
  void UpdateUI();

  // -- handle ui
  void HandleUI();

  // -- retrieve color id from buffer
  void RetrieveColorID();

  // -- load new models
  void LoadModel();

  // -- delete models
  void DeleteModel();

  // -- update renderer with ui data
  void UpdateRenderer();
};

} // namespace gtp
