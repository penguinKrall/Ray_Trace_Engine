#pragma once
#include <EngineCore.hpp>
#include <glTFModel.hpp>

class Utilities_AS {
public:
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

  struct GeometryData {
    std::vector<GeometryNode> geometryNodes;
    std::vector<GeometryIndex> geometryIndices;
  };

  struct AccelerationStructure {
    VkAccelerationStructureKHR accelerationStructureKHR;
    uint64_t deviceAddress = 0;
    VkDeviceMemory memory;
    VkBuffer buffer;
    gtp::Buffer scratchBuffer{};
  };

  struct BLASData {
    std::vector<GeometryNode> geometryNodes{};
    VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
    VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
    uint32_t maxPrimitiveCount{0};
    std::vector<uint32_t> maxPrimitiveCounts{};
    std::vector<VkAccelerationStructureGeometryKHR> geometries{};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos{};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR *> pBuildRangeInfos{};
    VkAccelerationStructureBuildGeometryInfoKHR
        accelerationStructureBuildGeometryInfo{};
    VkAccelerationStructureBuildSizesInfoKHR
        accelerationStructureBuildSizesInfo{};
    Utilities_AS::AccelerationStructure accelerationStructure{};
  };

  // -- TLAS data struct
  struct TLASData {
    VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
    VkAccelerationStructureBuildGeometryInfoKHR
        accelerationStructureBuildGeometryInfo{};
    VkAccelerationStructureBuildSizesInfoKHR
        accelerationStructureBuildSizesInfo{};
    VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
    VkAccelerationStructureBuildRangeInfoKHR
        accelerationStructureBuildRangeInfo{};
    std::vector<VkAccelerationStructureBuildRangeInfoKHR *>
        accelerationBuildStructureRangeInfos;
    VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
    uint32_t primitive_count = 0;
  };

  struct StorageImage {
    VkDeviceMemory memory;
    VkImage image;
    VkImageView view;
    VkFormat format;
  };

  static uint64_t getBufferDeviceAddress(EngineCore *pEngineCore,
                                         VkBuffer buffer);

  static void createScratchBuffer(EngineCore *pEngineCore, gtp::Buffer *buffer,
                                  VkDeviceSize size, std::string name);

  static void createAccelerationStructureBuffer(
      EngineCore *coreBase, VkDeviceMemory *memory, VkBuffer *buffer,
      VkAccelerationStructureBuildSizesInfoKHR *buildSizeInfo,
      std::string bufferName);

  static void createStorageImage(EngineCore *pEngineCore,
                                 StorageImage *storageImage, std::string name);

  static void
  createBLAS(EngineCore *pEngineCore,
             std::vector<Utilities_AS::GeometryNode> *geometryNodeBuf,
             std::vector<Utilities_AS::GeometryIndex> *geometryIndexBuf,
             Utilities_AS::BLASData *blasData,
             Utilities_AS::AccelerationStructure *BLAS, gtp::Model *model,
             uint32_t textureOffset);

  static std::vector<gtp::Model::Vertex>
  GetVerticesFromBuffer(VkDevice device, gtp::Model *model);
};
