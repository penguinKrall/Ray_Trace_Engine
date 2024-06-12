#pragma once

#include <Tools.hpp>
#include <Buffer.hpp>

// -- shader binding table class
class ShaderBindingTable : public vrt::Buffer {
public:

	VkStridedDeviceAddressRegionKHR stridedDeviceAddressRegion{};

};

