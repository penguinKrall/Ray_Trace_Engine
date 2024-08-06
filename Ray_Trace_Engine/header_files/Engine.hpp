#pragma once

#include <CoreUI.hpp>
#include <DefaultRenderer.hpp>
#include <EngineCore.hpp>
#include <Input.hpp>
#include <LoadingScreen.hpp>
#include <Player.hpp>
namespace gtp {

// -- engine
//@brief contains core objects and renderers
class Engine : private EngineCore {
private:
  uint32_t currentFrame = 0;

  float deltaTime = 0.0f;
  float lastTime = 0.0f;
  float timer = 0.0f;

  // -- input class
  gtp::Input *pInput = nullptr;

  // -- renderers
  struct Renderers {
    DefaultRenderer *defaultRenderer = nullptr;
  };
  Renderers renderers{};

  // -- beings
  struct Players {
    bool playerLoaded = false;
    gtp::Player *playerCharacter = nullptr;
  };

  Players players{};

  // Loading Screen
  gtp::LoadingScreen loadingScreen;

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
  Engine();

  // -- init engine
  //@brief calls inherited 'initCore' func and initializes renderers
  VkResult InitEngine();

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

  // -- handle resize
  void HandleResize();
};

} // namespace gtp
