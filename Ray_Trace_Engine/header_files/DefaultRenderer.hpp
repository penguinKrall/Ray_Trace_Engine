#pragma once
#include <RenderBase.hpp>

namespace gtp {

class DefaultRenderer : private RenderBase {
 private:
 public:
  // -- constructor
  DefaultRenderer(EngineCore* engineCorePtr);

  // -- destroy
  void Destroy();
};

}  // namespace gtp
