#pragma once

#include <Utilities_EngCore.hpp>
#include <Buffer.hpp>

// -- shader binding table class
class ShaderBindingTable : public gtp::Buffer {
public:

	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};

};

