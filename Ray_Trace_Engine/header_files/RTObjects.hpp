#pragma once
#include <CoreBase.hpp>
#include <VulkanglTFModel.h>

namespace vrt {

class RTObjects{
public:

	struct GeometryNode {
		uint64_t vertexBufferDeviceAddress = 0;
		uint64_t indexBufferDeviceAddress = 0;
		int textureIndexBaseColor = 0;
		int textureIndexOcclusion = 0;
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
		vrt::Buffer scratchBuffer{};
	};

	struct BLASData {
		std::vector<GeometryNode> geometryNodes{};
		VkDeviceOrHostAddressConstKHR vertexBufferDeviceAddress{};
		VkDeviceOrHostAddressConstKHR indexBufferDeviceAddress{};
		VkDeviceOrHostAddressConstKHR transformBufferDeviceAddress{};
		uint32_t maxPrimitiveCount{ 0 };
		std::vector<uint32_t> maxPrimitiveCounts{};
		std::vector<VkAccelerationStructureGeometryKHR> geometries{};
		std::vector<VkAccelerationStructureBuildRangeInfoKHR> buildRangeInfos{};
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> pBuildRangeInfos{};
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		vrt::RTObjects::AccelerationStructure accelerationStructure{};
	};


	// -- TLAS data struct
	struct TLASData {
		VkAccelerationStructureGeometryKHR accelerationStructureGeometry{};
		VkAccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo{};
		VkAccelerationStructureBuildSizesInfoKHR accelerationStructureBuildSizesInfo{};
		VkAccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo{};
		VkAccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo{};
		std::vector<VkAccelerationStructureBuildRangeInfoKHR*> accelerationBuildStructureRangeInfos;
		VkDeviceOrHostAddressConstKHR instanceDataDeviceAddress{};
		uint32_t primitive_count = 0;
	};


	struct StorageImage {
		VkDeviceMemory memory;
		VkImage image;
		VkImageView view;
		VkFormat format;
	};

	static uint64_t getBufferDeviceAddress(CoreBase* pCoreBase, VkBuffer buffer);

	static void createScratchBuffer(CoreBase* pCoreBase, vrt::Buffer* buffer, VkDeviceSize size, std::string name);

	static void createAccelerationStructureBuffer(CoreBase* coreBase, VkDeviceMemory* memory, VkBuffer* buffer,
		VkAccelerationStructureBuildSizesInfoKHR* buildSizeInfo, std::string bufferName);

	static void createStorageImage(CoreBase* pCoreBase, StorageImage* storageImage, std::string name);

	static void createBLAS(CoreBase* pCoreBase,
		std::vector<vrt::RTObjects::GeometryNode>* geometryNodeBuf,
		std::vector<vrt::RTObjects::GeometryIndex>* geometryIndexBuf,
		vrt::RTObjects::BLASData* blasData,
		vrt::RTObjects::AccelerationStructure* BLAS,
		vkglTF::Model* model,
		uint32_t textureOffset);
};

}
