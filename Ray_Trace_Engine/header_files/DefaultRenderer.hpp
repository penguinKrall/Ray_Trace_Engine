#pragma once
#include <RenderBase.hpp>

namespace gtp {

class DefaultRenderer : private RenderBase {
 private:
 public:
  DefaultRenderer(EngineCore* engineCorePtr);
  void Destroy();
};
}  // namespace gtp
