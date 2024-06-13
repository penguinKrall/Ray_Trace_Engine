#pragma once

#include <Buffer.hpp>
#include <Utilities_EngCore.hpp>

// -- shader binding table class
class ShaderBindingTable : public gtp::Buffer {
public:
  VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};
};
