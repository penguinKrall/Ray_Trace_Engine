#pragma once

// #include <ComputeVertex.hpp>
#include <EngineCore.hpp>
#include <Particle.hpp>
#include <Shader.hpp>
// #include <TextureLoader.hpp>
// #include <Utilities_AS.hpp>
// #include <Utilities_Renderer.hpp>
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

struct AccelerationStructure {
  VkAccelerationStructureKHR accelerationStructureKHR;
  uint64_t deviceAddress = 0;
  VkDeviceMemory memory;
  VkBuffer buffer;
  gtp::Buffer scratchBuffer{};
};

struct BottomLevelAccelerationStructureData {
  std::vector<GeometryNode> geometryNodes{};
  VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
  VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
  VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
  uint32_t maxPrimitiveCount{0};
  std::vector<uint32_t> maxPrimitiveCounts{};
  std::vector<VkAccelerationStructureGeometryKHR> geometries{};
  std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos{};
  std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos{};
  VkAccelerationStructureBuildGeometryInfoKHR
      accelerationStructureBuildGeometryInfo{};
  VkAccelerationStructureBuildSizesInfoKHR
      accelerationStructureBuildSizesInfo{};
  AccelerationStructure accelerationStructure{};
};

// -- Top Level Acceleration Structure data struct
struct TopLevelAccelerationStructureData {
  std::vector<VkAccelerationStructureInstanceKHR>
      bottomLevelAccelerationStructureInstances{};
  VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
  VkAccelerationStructureBuildGeometryInfoKHR
      accelerationStructureBuildGeometryInfo{};
  VkAccelerationStructureBuildSizesInfoKHR
      accelerationStructureBuildSizesInfo{};
  VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
  VkAccelerationStructureBuildRangeInfoKHR
      accelerationStructureBuildRangeInfo{};
  std::vector<VkAccelerationStructureBuildRangeInfoKHR*>
      accelerationBuildStructureRangeInfos;
  VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
  uint32_t primitive_count = 0;
};

class AccelerationStructures {
 private:
  /* vars */
   //flags
   bool updateTopLevelAS = false;
   bool updateBottomLevelAS = false;

  // buffers
  //  -- buffers
  struct Buffers {
    gtp::Buffer transformBuffer{};
    gtp::Buffer geometry_nodes_buffer{};
    gtp::Buffer geometry_nodes_indices{};
    gtp::Buffer blas_scratch{};
    gtp::Buffer tlas_scratch{};
    gtp::Buffer tlas_instancesBuffer{};
  };

  Buffers buffers{};

  // texture offset
  uint32_t textureOffset = 0;

  // geometry nodes
  std::vector<GeometryNode> geometryNodes;

  // geometry nodes index
  std::vector<GeometryIndex> geometryNodesIndex;

  // bottom level acceleration structure data
  //  -- bottom level acceleration structures
  std::vector<BottomLevelAccelerationStructureData*>
      bottomLevelAccelerationStructures;

  // -- top level acceleration structure
  AccelerationStructure topLevelAccelerationStructure{};
  TopLevelAccelerationStructureData topLevelAccelerationStructureData{};

  // core ptr
  EngineCore* pEngineCore = nullptr;

  /* funcs */
  // -- create scratch buffer
  void CreateScratchBuffer(gtp::Buffer& buffer, VkDeviceSize size,
                           std::string name);

  // -- create acceleration structure buffer
  void CreateAccelerationStructureBuffer(
      AccelerationStructure& accelerationStructure,
      VkAccelerationStructureBuildSizesInfoKHR* buildSizeInfo,
      std::string& bufferName);

  // -- create bottom level acceleration structure
  void CreateBottomLevelAccelerationStructure(gtp::Model* pModel);

  // -- create geometry nodes buffer
  void CreateGeometryNodesBuffer();

  // -- create top level acceleration structure
  void CreateTopLevelAccelerationStructure(
      std::vector<gtp::Model*>& pModelList, Utilities_UI::ModelData& modelData,
      std::vector<gtp::Particle*>& pParticleList);

  // -- init func
  void InitAccelerationStructures(EngineCore* engineCorePtr);

  // -- rebuild bottom level acceleration structure
  void RebuildBottomLevelAccelerationStructure(int modelCount, Utilities_UI::ModelData& modelData);

  // -- rebuild top level acceleration structure
  void RebuildTopLevelAccelerationStructure(Utilities_UI::ModelData& modelData);

 public:
  // -- build bottom level acceleration structure
  void BuildBottomLevelAccelerationStructure(gtp::Model* pModel);


  // -- build top level acceleration structure
  void BuildTopLevelAccelerationStructure(
      std::vector<gtp::Model*>& pModelList, Utilities_UI::ModelData& modelData,
      std::vector<gtp::Particle*>& pParticleList);

  // -- rebuild Acceleration structures
  void RebuildAccelerationStructures(int modelCount, Utilities_UI::ModelData& modelData);

  // -- rebuild Geometry buffer
  void RebuildGeometryBuffer(gtp::Model* pModel, std::vector<gtp::Model*>& pModelList, Utilities_UI::ModelData& modelData,
    std::vector<gtp::Particle*>& pParticleList);

  // -- build geometry nodes buffer
  void BuildGeometryNodesBuffer();
  
  // -- update geometry nodes buffer
  void UpdateGeometryNodesBuffer(gtp::Model* pModel, std::vector<gtp::Model*>& pModelList, Utilities_UI::ModelData& modelData,
    std::vector<gtp::Particle*>& pParticleList);

  // -- handle model delete bottom level
  void HandleModelDeleteBottomLevel(int defaultTexCount, std::vector<gtp::Model*>& modelList);

  // -- handle model delete top level
  void HandleModelDeleteTopLevel(std::vector<gtp::Model*>& modelList, Utilities_UI::ModelData& modelData,
    std::vector<gtp::Particle*>& pParticleList);

  // -- get accelerationStructureKHR
  VkAccelerationStructureKHR GetTopLevelAccelerationStructureKHR();

  // -- get geometry buffer descriptor info
  VkDescriptorBufferInfo GetGeometryNodesBufferDescriptor();

  // -- get geometry nodes index buffer descriptor info
  VkDescriptorBufferInfo GetGeometryNodesIndexBufferDescriptor();

  // -- set texture offset
  void SetTextureOffset(int offset);

  // -- get texture offset
  uint32_t GetTextureOffset();

  // -- clear bottom level AS instances
  void ClearBottomLevelAccelerationStructureInstances();

  // -- base ctor
  AccelerationStructures();

  // -- init ctor
  AccelerationStructures(EngineCore* engineCorePtr);

  // -- destroy
  void DestroyAccelerationStructures();
};

}  // namespace gtp
