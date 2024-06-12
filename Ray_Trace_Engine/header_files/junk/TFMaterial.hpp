#pragma once

#include <Tools.hpp>
#include <Texture.hpp>

#include <tiny_gltf.h>

namespace vrt {

	class TFMaterial {
	public:
		// -- core pointer
		CoreBase* pCoreBase = nullptr;

		// -- alpha modes
		enum AlphaMode { ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };


		// -- alpha mode decl
		AlphaMode alphaMode = ALPHAMODE_OPAQUE;

		// -- settings?
		float alphaCutoff = 1.0f;
		float metallicFactor = 1.0f;
		float roughnessFactor = 1.0f;
		glm::vec4 baseColorFactor = glm::vec4(1.0f);

		// -- textures
		vrt::Texture baseColorTexture = nullptr;
		vrt::Texture diffuseTexture = nullptr;
		vrt::Texture emissiveTexture = nullptr;
		vrt::Texture metallicRoughnessTexture = nullptr;
		vrt::Texture normalTexture = nullptr;
		vrt::Texture occlusionTexture = nullptr;
		vrt::Texture specularGlossinessTexture = nullptr;

		// -- descriptors
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		// -- default ctor
		TFMaterial();

		// -- init ctor
		TFMaterial(CoreBase* coreBase);

		// -- init func
		void InitMaterial(CoreBase* coreBase);

	};

}

