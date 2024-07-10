#include "AccelerationStructures.hpp"

void gtp::AccelerationStructures::InitAccelerationStructures(
    EngineCore *engineCorePtr) {
  // initialize core pointer
  this->pEngineCore = engineCorePtr;
}

gtp::AccelerationStructures::AccelerationStructures() {}

gtp::AccelerationStructures::AccelerationStructures(EngineCore *engineCorePtr) {
  // call init function
  this->InitAccelerationStructures(engineCorePtr);
}

void gtp::AccelerationStructures::DestroyAccelerationStructures() {

  // output success
  std::cout << "successfully destroyed acceleration structures class"
            << std::endl;
}
