#pragma once

#include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <Particle.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>
#include <glTFModel.hpp>

namespace gtp {

  struct GeometryNode {
    uint64_t vertexBufferDeviceAddress = 0;
    uint64_t indexBufferDeviceAddress = 0;
    int textureIndexBaseColor = -1;
    int textureIndexOcclusion = -1;
    int textureIndexMetallicRoughness = -1;
    int textureIndexNormal = -1;
    int textureIndexEmissive = -1;
    int semiTransparentFlag = 0;
    float objectIDColor;
  };

  struct GeometryIndex {
    int nodeOffset = -1;
  };

  struct AccelerationStructureData {
    std::vector<GeometryNode> geometryNodes{};
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    uint32_t maxPrimitiveCount{ 0 };
    std::vector<uint32_t> maxPrimitiveCounts{};
    std::vector<VkAccelerationStructureGeometryKHR> geometries{};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos{};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos{};
    VkAccelerationStructureBuildGeometryInfoKHR
      accelerationStructureBuildGeometryInfo{};
    VkAccelerationStructureBuildSizesInfoKHR
      accelerationStructureBuildSizesInfo{};
    Utilities_AS::AccelerationStructure accelerationStructure{};
  };

class AccelerationStructures {
private:
  /* vars */
  //texture offset
  uint32_t textureOffset = 0;
  //geometry nodes
  std::vector<GeometryNode> geometryNodes;

  void CreateBottomLevelAccelerationStructure();

  // core ptr
  EngineCore *pEngineCore = nullptr;

  /* funcs */

  // -- init func
  void InitAccelerationStructures(EngineCore *engineCorePtr);

public:
  // -- base ctor
  AccelerationStructures();

  // -- init ctor
  AccelerationStructures(EngineCore *engineCorePtr);

  // -- destroy
  void DestroyAccelerationStructures();
};

} // namespace gtp
