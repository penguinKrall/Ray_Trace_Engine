/*
* Vulkan glTF model and texture loading class based on tinyglTF (https://github.com/syoyo/tinygltf)
*
* Copyright (C) 2018-2023 by Sascha Willems - www.saschawillems.de
*
* This code is licensed under the MIT license (MIT) (http://opensource.org/licenses/MIT)
*/

/*
 * Note that this isn't a complete glTF loader and not all features of the glTF 2.0 spec are supported
 * For details on how glTF 2.0 works, see the official spec at https://github.com/KhronosGroup/glTF/tree/master/specification/2.0
 *
 * If you are looking for a complete glTF implementation, check out https://github.com/SaschaWillems/Vulkan-glTF-PBR/
 */
#include "VulkanglTFModel.h"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

 //#include <tiny_gltf.h>


VkDescriptorSetLayout vkglTF::descriptorSetLayoutImage = VK_NULL_HANDLE;
VkDescriptorSetLayout vkglTF::descriptorSetLayoutUbo = VK_NULL_HANDLE;
VkMemoryPropertyFlags vkglTF::memoryPropertyFlags = 0;
uint32_t vkglTF::descriptorBindingFlags = vkglTF::DescriptorBindingFlags::ImageBaseColor;

/*
	We use a custom image loading function with tinyglTF, so we can do custom stuff loading ktx textures
*/
bool vkgltf_loadImageDataFunc(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData)
{
	// KTX files will be handled by our own code
	if (image->uri.find_last_of(".") != std::string::npos) {
		if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx") {
			return true;
		}
	}

	return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
}

bool vkgltf_loadImageDataFuncEmpty(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData)
{
	// This function will be used for samples that don't require images to be loaded
	return true;
}


/*
	glTF texture loading class
*/

void vkglTF::Texture::updateDescriptor()
{
	descriptor.sampler = sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = imageLayout;
}

void vkglTF::Texture::destroy()
{
	if (pCoreBase)
	{
		vkDestroyImageView(pCoreBase->devices.logical, view, nullptr);
		vkDestroyImage(pCoreBase->devices.logical, image, nullptr);
		vkFreeMemory(pCoreBase->devices.logical, deviceMemory, nullptr);
		vkDestroySampler(pCoreBase->devices.logical, sampler, nullptr);
	}
}

void vkglTF::Texture::fromglTfImage(tinygltf::Image& gltfimage, std::string path, CoreBase* coreBase, VkQueue copyQueue)
{
	this->pCoreBase = coreBase;

	bool isKtx = false;
	// Image points to an external ktx file
	if (gltfimage.uri.find_last_of(".") != std::string::npos) {
		if (gltfimage.uri.substr(gltfimage.uri.find_last_of(".") + 1) == "ktx") {
			isKtx = true;
		}
	}

	VkFormat format;

	if (!isKtx) {
		// Texture was loaded using STB_Image

		unsigned char* buffer = nullptr;
		VkDeviceSize bufferSize = 0;
		bool deleteBuffer = false;
		if (gltfimage.component == 3) {
			// Most devices don't support RGB only on Vulkan so convert if necessary
			// TODO: Check actual format support and transform only if required
			bufferSize = gltfimage.width * gltfimage.height * 4;
			buffer = new unsigned char[bufferSize];
			unsigned char* rgba = buffer;
			unsigned char* rgb = &gltfimage.image[0];
			for (size_t i = 0; i < gltfimage.width * gltfimage.height; ++i) {
				for (int32_t j = 0; j < 3; ++j) {
					rgba[j] = rgb[j];
				}
				rgba += 4;
				rgb += 3;
			}
			deleteBuffer = true;
		}
		else {
			buffer = &gltfimage.image[0];
			bufferSize = gltfimage.image.size();
		}

		format = VK_FORMAT_R8G8B8A8_UNORM;

		VkFormatProperties formatProperties;

		width = gltfimage.width;
		height = gltfimage.height;
		mipLevels = static_cast<uint32_t>(floor(log2(std::max(width, height))) + 1.0);

		vkGetPhysicalDeviceFormatProperties(pCoreBase->devices.physical, format, &formatProperties);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT);
		assert(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs{};

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = bufferSize;
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(pCoreBase->devices.logical, &bufferCreateInfo, nullptr, &stagingBuffer));
		vkGetBufferMemoryRequirements(pCoreBase->devices.logical, stagingBuffer, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &stagingMemory));
		if (vkBindBufferMemory(pCoreBase->devices.logical, stagingBuffer, stagingMemory, 0));

		uint8_t* data;
		if (vkMapMemory(pCoreBase->devices.logical, stagingMemory, 0, memReqs.size, 0, (void**)&data));
		memcpy(data, buffer, bufferSize);
		vkUnmapMemory(pCoreBase->devices.logical, stagingMemory);

		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		if (vkCreateImage(pCoreBase->devices.logical, &imageCreateInfo, nullptr, &image));
		vkGetImageMemoryRequirements(pCoreBase->devices.logical, image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &deviceMemory));
		if (vkBindImageMemory(pCoreBase->devices.logical, image, deviceMemory, 0));

		VkCommandBuffer copyCmd = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 1;

		VkImageMemoryBarrier imageMemoryBarrier{};

		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.srcAccessMask = 0;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

		VkBufferImageCopy bufferCopyRegion = {};
		bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		bufferCopyRegion.imageSubresource.mipLevel = 0;
		bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
		bufferCopyRegion.imageSubresource.layerCount = 1;
		bufferCopyRegion.imageExtent.width = width;
		bufferCopyRegion.imageExtent.height = height;
		bufferCopyRegion.imageExtent.depth = 1;

		vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

		pCoreBase->FlushCommandBuffer(copyCmd, copyQueue, pCoreBase->commandPools.graphics, true);

		vkDestroyBuffer(pCoreBase->devices.logical, stagingBuffer, nullptr);
		vkFreeMemory(pCoreBase->devices.logical, stagingMemory, nullptr);

		// Generate the mip chain (glTF uses jpg and png, so we need to create this manually)
		VkCommandBuffer blitCmd = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		for (uint32_t i = 1; i < mipLevels; i++) {
			VkImageBlit imageBlit{};

			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcOffsets[1].x = int32_t(width >> (i - 1));
			imageBlit.srcOffsets[1].y = int32_t(height >> (i - 1));
			imageBlit.srcOffsets[1].z = 1;

			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstOffsets[1].x = int32_t(width >> i);
			imageBlit.dstOffsets[1].y = int32_t(height >> i);
			imageBlit.dstOffsets[1].z = 1;

			VkImageSubresourceRange mipSubRange = {};
			mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			mipSubRange.baseMipLevel = i;
			mipSubRange.levelCount = 1;
			mipSubRange.layerCount = 1;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = mipSubRange;
				vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			vkCmdBlitImage(blitCmd, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageBlit, VK_FILTER_LINEAR);

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = mipSubRange;
				vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}
		}

		subresourceRange.levelCount = mipLevels;
		imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		imageMemoryBarrier.image = image;
		imageMemoryBarrier.subresourceRange = subresourceRange;
		vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

		if (deleteBuffer) {
			delete[] buffer;
		}

		pCoreBase->FlushCommandBuffer(blitCmd, copyQueue, pCoreBase->commandPools.graphics, true);
	}
	else {
		// Texture is stored in an external ktx file
		std::string filename = path + "/" + gltfimage.uri;

		ktxTexture* ktxTexture;

		ktxResult result = KTX_SUCCESS;
#if defined(__ANDROID__)
		AAsset* asset = AAssetManager_open(androidApp->activity->assetManager, filename.c_str(), AASSET_MODE_STREAMING);
		if (!asset) {
			vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		}
		size_t size = AAsset_getLength(asset);
		assert(size > 0);
		ktx_uint8_t* textureData = new ktx_uint8_t[size];
		AAsset_read(asset, textureData, size);
		AAsset_close(asset);
		result = ktxTexture_CreateFromMemory(textureData, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
		delete[] textureData;
#else
		//if (!vks::tools::fileExists(filename)) {
		//	vks::tools::exitFatal("Could not load texture from " + filename + "\n\nMake sure the assets submodule has been checked out and is up-to-date.", -1);
		//}
		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
#endif		
		assert(result == KTX_SUCCESS);

		this->pCoreBase = coreBase;
		width = ktxTexture->baseWidth;
		height = ktxTexture->baseHeight;
		mipLevels = ktxTexture->numLevels;

		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(ktxTexture);
		// @todo: Use ktxTexture_GetVkFormat(ktxTexture)
		format = VK_FORMAT_R8G8B8A8_UNORM;

		// Get device properties for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(pCoreBase->devices.physical, format, &formatProperties);

		VkCommandBuffer copyCmd = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingMemory;

		VkBufferCreateInfo bufferCreateInfo{};
		bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferCreateInfo.size = ktxTextureSize;
		// This buffer is used as a transfer source for the buffer copy
		bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateBuffer(pCoreBase->devices.logical, &bufferCreateInfo, nullptr, &stagingBuffer));

		VkMemoryAllocateInfo memAllocInfo{};
		memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		VkMemoryRequirements memReqs;
		vkGetBufferMemoryRequirements(pCoreBase->devices.logical, stagingBuffer, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &stagingMemory));
		if (vkBindBufferMemory(pCoreBase->devices.logical, stagingBuffer, stagingMemory, 0));

		uint8_t* data;
		if (vkMapMemory(pCoreBase->devices.logical, stagingMemory, 0, memReqs.size, 0, (void**)&data));
		memcpy(data, ktxTextureData, ktxTextureSize);
		vkUnmapMemory(pCoreBase->devices.logical, stagingMemory);

		std::vector<VkBufferImageCopy> bufferCopyRegions;
		for (uint32_t i = 0; i < mipLevels; i++)
		{
			ktx_size_t offset;
			KTX_error_code result = ktxTexture_GetImageOffset(ktxTexture, i, 0, 0, &offset);
			assert(result == KTX_SUCCESS);
			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = i;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = std::max(1u, ktxTexture->baseWidth >> i);
			bufferCopyRegion.imageExtent.height = std::max(1u, ktxTexture->baseHeight >> i);
			bufferCopyRegion.imageExtent.depth = 1;
			bufferCopyRegion.bufferOffset = offset;
			bufferCopyRegions.push_back(bufferCopyRegion);
		}

		// Create optimal tiled target image
		VkImageCreateInfo imageCreateInfo{};
		imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
		imageCreateInfo.format = format;
		imageCreateInfo.mipLevels = mipLevels;
		imageCreateInfo.arrayLayers = 1;
		imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageCreateInfo.extent = { width, height, 1 };
		imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
		if (vkCreateImage(pCoreBase->devices.logical, &imageCreateInfo, nullptr, &image));

		vkGetImageMemoryRequirements(pCoreBase->devices.logical, image, &memReqs);
		memAllocInfo.allocationSize = memReqs.size;
		memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &deviceMemory));
		if (vkBindImageMemory(pCoreBase->devices.logical, image, deviceMemory, 0));

		VkImageSubresourceRange subresourceRange = {};
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.baseMipLevel = 0;
		subresourceRange.levelCount = mipLevels;
		subresourceRange.layerCount = 1;

		vrt::Tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
		vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

		vrt::Tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
		pCoreBase->FlushCommandBuffer(copyCmd, copyQueue, pCoreBase->commandPools.graphics, true);
		this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		vkDestroyBuffer(pCoreBase->devices.logical, stagingBuffer, nullptr);
		vkFreeMemory(pCoreBase->devices.logical, stagingMemory, nullptr);

		ktxTexture_Destroy(ktxTexture);
	}

	VkSamplerCreateInfo samplerInfo{};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	samplerInfo.maxAnisotropy = 1.0;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.maxLod = (float)mipLevels;
	samplerInfo.maxAnisotropy = 8.0f;
	samplerInfo.anisotropyEnable = VK_TRUE;
	if (vkCreateSampler(pCoreBase->devices.logical, &samplerInfo, nullptr, &sampler));

	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewInfo.subresourceRange.layerCount = 1;
	viewInfo.subresourceRange.levelCount = mipLevels;
	if (vkCreateImageView(pCoreBase->devices.logical, &viewInfo, nullptr, &view));

	descriptor.sampler = sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = imageLayout;
}

/*
	glTF material
*/
void vkglTF::Material::createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags)
{
	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = descriptorPool;
	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
	descriptorSetAllocInfo.descriptorSetCount = 1;
	if (vkAllocateDescriptorSets(pCoreBase->devices.logical, &descriptorSetAllocInfo, &descriptorSet));
	std::vector<VkDescriptorImageInfo> imageDescriptors{};
	std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
	if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
		imageDescriptors.push_back(baseColorTexture->descriptor);
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
		writeDescriptorSet.pImageInfo = &baseColorTexture->descriptor;
		writeDescriptorSets.push_back(writeDescriptorSet);
	}
	if (normalTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
		imageDescriptors.push_back(normalTexture->descriptor);
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
		writeDescriptorSet.pImageInfo = &normalTexture->descriptor;
		writeDescriptorSets.push_back(writeDescriptorSet);
	}
	vkUpdateDescriptorSets(pCoreBase->devices.logical, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
}


/*
	glTF primitive
*/
void vkglTF::Primitive::setDimensions(glm::vec3 min, glm::vec3 max) {
	dimensions.min = min;
	dimensions.max = max;
	dimensions.size = max - min;
	dimensions.center = (min + max) / 2.0f;
	dimensions.radius = glm::distance(min, max) / 2.0f;
}

/*
	glTF mesh
*/
vkglTF::Mesh::Mesh(CoreBase* coreBase, glm::mat4 matrix) {
	this->pCoreBase = coreBase;
	this->uniformBlock.matrix = matrix;
	if (coreBase->CreateBuffer(
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&uniformBuffer.buffer,
		sizeof(uniformBlock),
		&uniformBlock));

	//if (vkMapMemory(pCoreBase->devices.logical, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
	//uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
};

vkglTF::Mesh::~Mesh() {
	vkDestroyBuffer(pCoreBase->devices.logical, uniformBuffer.buffer.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, uniformBuffer.buffer.bufferData.memory, nullptr);
	for (auto primitive : primitives)
	{
		delete primitive;
	}
}

/*
	glTF node
*/
glm::mat4 vkglTF::Node::localMatrix() {
	return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
}

glm::mat4 vkglTF::Node::getMatrix() {
	glm::mat4 m = localMatrix();
	vkglTF::Node* p = parent;
	while (p) {
		m = p->localMatrix() * m;
		p = p->parent;
	}
	return m;
}

void vkglTF::Node::update() {

	//std::cout << "update node" << std::endl;

	if (mesh) {

		glm::mat4 m = getMatrix();

		if (skin) {

			mesh->uniformBlock.matrix = m;

			// Update join matrices
			glm::mat4 inverseTransform = glm::inverse(m);

			for (size_t i = 0; i < skin->joints.size(); i++) {

				vkglTF::Node* jointNode = skin->joints[i];

				glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];

				jointMat = inverseTransform * jointMat;

				mesh->uniformBlock.jointMatrix[i] = jointMat;

				if (this->transformMatrices.empty()) {
					this->transformMatrices.resize(skin->joints.size());
				}

				this->transformMatrices[i] = jointMat;

			}

			mesh->uniformBlock.jointcount = (float)skin->joints.size();
			memcpy(mesh->uniformBuffer.buffer.bufferData.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
		}

		else {
			memcpy(mesh->uniformBuffer.buffer.bufferData.mapped, &m, sizeof(glm::mat4));
		}

	}

	for (auto& child : children) {
		child->update();
	}
}

vkglTF::Node::~Node() {
	if (mesh) {
		delete mesh;
	}
	for (auto& child : children) {
		delete child;
	}
}

/*
	glTF default vertex layout with easy Vulkan mapping functions
*/

//VkVertexInputBindingDescription vkglTF::Vertex::vertexInputBindingDescription;
//std::vector<VkVertexInputAttributeDescription> vkglTF::Vertex::vertexInputAttributeDescriptions;
//VkPipelineVertexInputStateCreateInfo vkglTF::Vertex::pipelineVertexInputStateCreateInfo;
//
//VkVertexInputBindingDescription vkglTF::Vertex::inputBindingDescription(uint32_t binding) {
//	return VkVertexInputBindingDescription({ binding, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX });
//}
//
//VkVertexInputAttributeDescription vkglTF::Vertex::inputAttributeDescription(uint32_t binding, uint32_t location, VertexComponent component) {
//	switch (component) {
//	case VertexComponent::Position:
//		return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos) });
//	case VertexComponent::Normal:
//		return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) });
//	case VertexComponent::UV:
//		return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) });
//	case VertexComponent::Color:
//		return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) });
//	case VertexComponent::Tangent:
//		return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, tangent) });
//	case VertexComponent::Joint0:
//		return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, joint0) });
//	case VertexComponent::Weight0:
//		return VkVertexInputAttributeDescription({ location, binding, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, weight0) });
//	default:
//		return VkVertexInputAttributeDescription({});
//	}
//}
//
//std::vector<VkVertexInputAttributeDescription> vkglTF::Vertex::inputAttributeDescriptions(uint32_t binding, const std::vector<VertexComponent> components) {
//	std::vector<VkVertexInputAttributeDescription> result;
//	uint32_t location = 0;
//	for (VertexComponent component : components) {
//		result.push_back(Vertex::inputAttributeDescription(binding, location, component));
//		location++;
//	}
//	return result;
//}
//
///** @brief Returns the default pipeline vertex input state create info structure for the requested vertex components */
//VkPipelineVertexInputStateCreateInfo* vkglTF::Vertex::getPipelineVertexInputState(const std::vector<VertexComponent> components) {
//	vertexInputBindingDescription = Vertex::inputBindingDescription(0);
//	Vertex::vertexInputAttributeDescriptions = Vertex::inputAttributeDescriptions(0, components);
//	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//	pipelineVertexInputStateCreateInfo.vertexBindingDescriptionCount = 1;
//	pipelineVertexInputStateCreateInfo.pVertexBindingDescriptions = &Vertex::vertexInputBindingDescription;
//	pipelineVertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(Vertex::vertexInputAttributeDescriptions.size());
//	pipelineVertexInputStateCreateInfo.pVertexAttributeDescriptions = Vertex::vertexInputAttributeDescriptions.data();
//	return &pipelineVertexInputStateCreateInfo;
//}

vkglTF::Texture* vkglTF::Model::getTexture(uint32_t index)
{

	if (index < textures.size()) {
		return &textures[index];
	}
	return nullptr;
}

void vkglTF::Model::createEmptyTexture(VkQueue transferQueue)
{
	emptyTexture.pCoreBase = this->pCoreBase;
	emptyTexture.width = 1;
	emptyTexture.height = 1;
	emptyTexture.layerCount = 1;
	emptyTexture.mipLevels = 1;

	size_t bufferSize = emptyTexture.width * emptyTexture.height * 4;
	unsigned char* buffer = new unsigned char[bufferSize];
	memset(buffer, 0, bufferSize);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingMemory;
	VkBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = bufferSize;
	// This buffer is used as a transfer source for the buffer copy
	bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(pCoreBase->devices.logical, &bufferCreateInfo, nullptr, &stagingBuffer));

	VkMemoryAllocateInfo memAllocInfo{};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetBufferMemoryRequirements(pCoreBase->devices.logical, stagingBuffer, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &stagingMemory));
	if (vkBindBufferMemory(pCoreBase->devices.logical, stagingBuffer, stagingMemory, 0));

	// Copy texture data into staging buffer
	uint8_t* data;
	if (vkMapMemory(pCoreBase->devices.logical, stagingMemory, 0, memReqs.size, 0, (void**)&data));
	memcpy(data, buffer, bufferSize);
	vkUnmapMemory(pCoreBase->devices.logical, stagingMemory);

	VkBufferImageCopy bufferCopyRegion = {};
	bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	bufferCopyRegion.imageSubresource.layerCount = 1;
	bufferCopyRegion.imageExtent.width = emptyTexture.width;
	bufferCopyRegion.imageExtent.height = emptyTexture.height;
	bufferCopyRegion.imageExtent.depth = 1;

	// Create optimal tiled target image
	VkImageCreateInfo imageCreateInfo{};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.extent = { emptyTexture.width, emptyTexture.height, 1 };
	imageCreateInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	if (vkCreateImage(pCoreBase->devices.logical, &imageCreateInfo, nullptr, &emptyTexture.image));

	vkGetImageMemoryRequirements(pCoreBase->devices.logical, emptyTexture.image, &memReqs);
	memAllocInfo.allocationSize = memReqs.size;
	memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &emptyTexture.deviceMemory));
	if (vkBindImageMemory(pCoreBase->devices.logical, emptyTexture.image, emptyTexture.deviceMemory, 0));

	VkImageSubresourceRange subresourceRange{};
	subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	subresourceRange.baseMipLevel = 0;
	subresourceRange.levelCount = 1;
	subresourceRange.layerCount = 1;

	VkCommandBuffer copyCmd = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
	vrt::Tools::setImageLayout(copyCmd, emptyTexture.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	vkCmdCopyBufferToImage(copyCmd, stagingBuffer, emptyTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);
	vrt::Tools::setImageLayout(copyCmd, emptyTexture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
	pCoreBase->FlushCommandBuffer(copyCmd, transferQueue, pCoreBase->commandPools.graphics, true);
	emptyTexture.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	// Clean up staging resources
	vkDestroyBuffer(pCoreBase->devices.logical, stagingBuffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, stagingMemory, nullptr);

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.maxAnisotropy = 1.0f;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerCreateInfo.maxAnisotropy = 1.0f;

	if (vkCreateSampler(pCoreBase->devices.logical, &samplerCreateInfo, nullptr, &emptyTexture.sampler) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create vulkangltfmodel sampler");
	}

	VkImageViewCreateInfo viewCreateInfo{};
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.image = emptyTexture.image;
	if (vkCreateImageView(pCoreBase->devices.logical, &viewCreateInfo, nullptr, &emptyTexture.view));

	emptyTexture.descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	emptyTexture.descriptor.imageView = emptyTexture.view;
	emptyTexture.descriptor.sampler = emptyTexture.sampler;
}

vkglTF::Model::Model(CoreBase* coreBase) {
	this->pCoreBase = coreBase;
}

void vkglTF::Model::UpdateVertexBuffer(glm::mat4 cameraView, glm::mat4 projection) {
	// Iterate over each node
	for (auto& node : this->linearNodes) {
		if (node->mesh) {

			/*glm::mat4 nodeMatrix = node->matrix;
			vkglTF::Node* currentParent = node->parent;
			while (currentParent)
			{
				nodeMatrix = currentParent->matrix * nodeMatrix;
				currentParent = currentParent->parent;
			}*/

			// Access the index buffer of the mesh
			const std::vector<uint32_t>& indices = modelIndexBuffer;
			//std::cout << "indices.size: " << indices.size() << std::endl;

			// Iterate over each vertex
			for (size_t i = 0; i < indices.size(); ++i) {
				uint32_t index = indices[i];
				glm::vec4 tempPos = tempVertexBuffer[index].pos;

				// Update the position and normal based on joint transformations
				glm::ivec4 vJoints = glm::ivec4(int(tempVertexBuffer[index].joint0.x), int(tempVertexBuffer[index].joint0.y), int(tempVertexBuffer[index].joint0.z), int(tempVertexBuffer[index].joint0.w));
				glm::vec4 vWeights = tempVertexBuffer[index].weight0;
				glm::mat4 transform = vWeights.x * node->transformMatrices[vJoints.x] +
					vWeights.y * node->transformMatrices[vJoints.y] +
					vWeights.z * node->transformMatrices[vJoints.z] +
					vWeights.w * node->transformMatrices[vJoints.w];


				glm::vec4 newPosition = transform * tempVertexBuffer[index].pos;
				modelVertexBuffer[index].pos = glm::vec4(newPosition.x, newPosition.y, newPosition.z, newPosition.w);
				modelVertexBuffer[index].normal = (transform * tempVertexBuffer[index].normal);
			}
		}
	}

	// Calculate the size of the updated vertex data
	size_t updatedVertexBufferSize = modelVertexBuffer.size() * sizeof(Vertex);

	// Create a staging buffer for the updated vertex data
	vrt::Buffer updatedVertexStaging{};
	updatedVertexStaging.bufferData.bufferName = "_updatedVertexStagingBuffer";
	updatedVertexStaging.bufferData.bufferMemoryName = "_updatedVertexStagingBufferMemory";

	// Create staging buffer for the updated vertex data
	VkResult result = pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&updatedVertexStaging,
		updatedVertexBufferSize,
		modelVertexBuffer.data());

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create updated vertex staging buffer");
	}

	// Copy the updated vertex data from the staging buffer to the device local vertex buffer
	VkCommandBuffer copyCmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};
	copyRegion.size = updatedVertexBufferSize;  // Size in bytes

	vkCmdCopyBuffer(copyCmdBuffer, updatedVertexStaging.bufferData.buffer, vertices.buffer.bufferData.buffer, 1, &copyRegion);

	// Submit the copy command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(copyCmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	// Destroy the staging buffer for updated vertex data
	vkDestroyBuffer(pCoreBase->devices.logical, updatedVertexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, updatedVertexStaging.bufferData.memory, nullptr);
}

void vkglTF::Model::UpdateVertexBuffer_2(std::vector<vkglTF::Vertex> newVertices) {

	// Calculate the size of the updated vertex data
	VkDeviceSize updatedVertexBufferSize = newVertices.size() * sizeof(vkglTF::Vertex);

	// Create a staging buffer for the updated vertex data
	vrt::Buffer updatedVertexStaging{};
	updatedVertexStaging.bufferData.bufferName = "_updatedVertexStagingBuffer";
	updatedVertexStaging.bufferData.bufferMemoryName = "_updatedVertexStagingBufferMemory";

	// Create staging buffer for the updated vertex data
	VkResult result = pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&updatedVertexStaging,
		updatedVertexBufferSize,
		newVertices.data());

	if (result != VK_SUCCESS) {
		throw std::runtime_error("Failed to create updated vertex staging buffer");
	}

	// Copy the updated vertex data from the staging buffer to the device local vertex buffer
	VkCommandBuffer copyCmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};
	copyRegion.size = updatedVertexBufferSize;  // Size in bytes
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;

	vkCmdCopyBuffer(copyCmdBuffer, updatedVertexStaging.bufferData.buffer, vertices.buffer.bufferData.buffer, 1, &copyRegion);

	// Submit the copy command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(copyCmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	// Destroy the staging buffer for updated vertex data
	vkDestroyBuffer(pCoreBase->devices.logical, updatedVertexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, updatedVertexStaging.bufferData.memory, nullptr);
}



/*
	glTF model loading and rendering class
*/
vkglTF::Model::~Model()
{
	//vkDestroyBuffer(pCoreBase->devices.logical, vertices.buffer.bufferData.buffer, nullptr);
	//vkFreeMemory(pCoreBase->devices.logical, vertices.buffer.bufferData.memory, nullptr);
	//vkDestroyBuffer(pCoreBase->devices.logical, indices.buffer.bufferData.buffer, nullptr);
	//vkFreeMemory(pCoreBase->devices.logical, indices.buffer.bufferData.memory, nullptr);
	//for (auto texture : textures) {
	//	texture.destroy();
	//}
	//for (auto node : nodes) {
	//	delete node;
	//}
	//for (auto skin : skins) {
	//	delete skin;
	//}
	//if (descriptorSetLayoutUbo != VK_NULL_HANDLE) {
	//	vkDestroyDescriptorSetLayout(pCoreBase->devices.logical, descriptorSetLayoutUbo, nullptr);
	//	descriptorSetLayoutUbo = VK_NULL_HANDLE;
	//}
	//if (descriptorSetLayoutImage != VK_NULL_HANDLE) {
	//	vkDestroyDescriptorSetLayout(pCoreBase->devices.logical, descriptorSetLayoutImage, nullptr);
	//	descriptorSetLayoutImage = VK_NULL_HANDLE;
	//}
	//vkDestroyDescriptorPool(pCoreBase->devices.logical, descriptorPool, nullptr);
	//emptyTexture.destroy();
}

void vkglTF::Model::loadNode(vkglTF::Node* parent, const tinygltf::Node& node, uint32_t nodeIndex,
	const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale) {

	std::cout << "\nload node\n----------\n" << std::endl;

	vkglTF::Node* newNode = new Node{};
	newNode->index = nodeIndex;
	newNode->parent = parent;
	newNode->name = node.name;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	std::cout << "node name: " << newNode->name << std::endl;

	// Generate local node matrix
	glm::vec3 translation = glm::vec3(0.0f);
	if (node.translation.size() == 3) {
		translation = glm::make_vec3(node.translation.data());
		newNode->translation = translation;
	}
	glm::mat4 rotation = glm::mat4(1.0f);
	if (node.rotation.size() == 4) {
		glm::quat q = glm::make_quat(node.rotation.data());
		newNode->rotation = glm::mat4(q);
	}
	glm::vec3 scale = glm::vec3(1.0f);
	if (node.scale.size() == 3) {
		scale = glm::make_vec3(node.scale.data());
		newNode->scale = scale;
	}
	if (node.matrix.size() == 16) {
		newNode->matrix = glm::make_mat4x4(node.matrix.data());
		if (globalscale != 1.0f) {
			//newNode->matrix = glm::scale(newNode->matrix, glm::vec3(globalscale));
		}
	};


	// Node with children
	if (node.children.size() > 0) {
		std::cout << "node " << newNode->name << " children count: " << newNode->children.size() << std::endl;
		for (auto i = 0; i < node.children.size(); i++) {
			loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
		}
	}

	//std::cout << "node child test 2" << std::endl;

	// Node contains mesh data
	if (node.mesh > -1) {
		const tinygltf::Mesh mesh = model.meshes[node.mesh];
		Mesh* newMesh = new Mesh(pCoreBase, newNode->matrix);
		newMesh->name = mesh.name;
		for (size_t j = 0; j < mesh.primitives.size(); j++) {
			const tinygltf::Primitive& primitive = mesh.primitives[j];
			if (primitive.indices < 0) {
				continue;
			}
			uint32_t indexStart = static_cast<uint32_t>(indexBuffer.size());
			uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t indexCount = 0;
			uint32_t vertexCount = 0;
			glm::vec3 posMin{};
			glm::vec3 posMax{};
			bool hasSkin = false;
			std::cout << "node: " << node.name << "\nindex start: " << indexStart << "\nvertex start: " << vertexStart << std::endl;
			// Vertices
			{
				const float* bufferPos = nullptr;
				const float* bufferNormals = nullptr;
				const float* bufferTexCoords = nullptr;
				const float* bufferColors = nullptr;
				const float* bufferTangents = nullptr;
				uint32_t numColorComponents;
				const uint16_t* bufferJoints = nullptr;
				const float* bufferWeights = nullptr;

				// Position attribute is required
				assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

				const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
				const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
				bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
				posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
					const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
					bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
				}

				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferTexCoords = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
				}

				if (primitive.attributes.find("COLOR_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
					const tinygltf::BufferView& colorView = model.bufferViews[colorAccessor.bufferView];
					// Color buffer are either of type vec3 or vec4
					numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
					bufferColors = reinterpret_cast<const float*>(&(model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
				}

				// Skinning
				// Joints
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
					bufferJoints = reinterpret_cast<const uint16_t*>(&(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]));
				}

				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferWeights = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
				}

				if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
				{
					const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& tangentView = model.bufferViews[tangentAccessor.bufferView];
					bufferTangents = reinterpret_cast<const float*>(&(model.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
				}

				hasSkin = (bufferJoints && bufferWeights);

				vertexCount = static_cast<uint32_t>(posAccessor.count);

				for (size_t v = 0; v < posAccessor.count; v++) {
					Vertex vert{};
					vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
					vert.normal = glm::normalize(glm::vec4(bufferNormals ? glm::vec4(glm::make_vec3(&bufferNormals[v * 3]), 1.0f) : glm::vec4(1.0f)));
					vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec2(1.0f);
					if (bufferColors) {
						switch (numColorComponents) {
						case 3:
							vert.color = glm::vec4(glm::make_vec3(&bufferColors[v * 3]), 1.0f);
						case 4:
							vert.color = glm::make_vec4(&bufferColors[v * 4]);
						}
					}
					else {
						vert.color = glm::vec4(1.0f);
					}
					vert.tangent = bufferTangents ? glm::vec4(glm::make_vec4(&bufferTangents[v * 4])) : glm::vec4(0.0f);
					vert.joint0 = hasSkin ? glm::vec4(glm::make_vec4(&bufferJoints[v * 4])) : glm::vec4(0.0f);
					vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * 4]) : glm::vec4(0.0f);
					vertexBuffer.push_back(vert);
				}
			}
			// Indices
			{
				const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);

				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					uint32_t* buf = new uint32_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					delete[] buf;
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					uint16_t* buf = new uint16_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					delete[] buf;
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					uint8_t* buf = new uint8_t[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
					for (size_t index = 0; index < accessor.count; index++) {
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					delete[] buf;
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return;
				}
			}
			Primitive* newPrimitive = new Primitive(indexStart, indexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
			newPrimitive->firstVertex = vertexStart;
			newPrimitive->vertexCount = vertexCount;
			newPrimitive->setDimensions(posMin, posMax);
			newMesh->primitives.push_back(newPrimitive);
			std::cout << "************************ node " << newNode->name << " pushed back primitive" << std::endl;
			std::cout << "newPrimitive->firstVertex " << newPrimitive->firstVertex << std::endl;
			std::cout << "newPrimitive->vertexCount " << newPrimitive->vertexCount << std::endl;
		}
		newNode->mesh = newMesh;
	}
	if (parent) {
		std::cout << "parent name: " << parent->name << std::endl;
		std::cout << "new node name: " << newNode->name << std::endl;
		if (newNode->mesh != nullptr) {
			std::cout << newNode->name << " node has mesh " << std::endl;
		}
		parent->children.push_back(newNode);
	}

	else {
		nodes.push_back(newNode);
	}

	linearNodes.push_back(newNode);
}

void vkglTF::Model::loadSkins(tinygltf::Model& gltfModel)
{
	for (tinygltf::Skin& source : gltfModel.skins) {
		Skin* newSkin = new Skin{};
		newSkin->name = source.name;

		// Find skeleton root node
		if (source.skeleton > -1) {
			newSkin->skeletonRoot = nodeFromIndex(source.skeleton);
		}

		// Find joint nodes
		for (int jointIndex : source.joints) {
			Node* node = nodeFromIndex(jointIndex);
			if (node) {
				newSkin->joints.push_back(nodeFromIndex(jointIndex));
			}
		}

		// Get inverse bind matrices from buffer
		if (source.inverseBindMatrices > -1) {
			const tinygltf::Accessor& accessor = gltfModel.accessors[source.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];
			newSkin->inverseBindMatrices.resize(accessor.count);
			memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
		}

		skins.push_back(newSkin);
	}
}

void vkglTF::Model::loadImages(tinygltf::Model& gltfModel, CoreBase* coreBase, VkQueue transferQueue)
{
	for (tinygltf::Image& image : gltfModel.images) {
		vkglTF::Texture texture;
		texture.fromglTfImage(image, path, pCoreBase, transferQueue);
		texture.index = static_cast<uint32_t>(textures.size());
		textures.push_back(texture);
	}
	// Create an empty texture to be used for empty material images
	createEmptyTexture(transferQueue);
}

void vkglTF::Model::loadMaterials(tinygltf::Model& gltfModel)
{
	for (tinygltf::Material& mat : gltfModel.materials) {
		vkglTF::Material material(pCoreBase);
		if (mat.values.find("baseColorTexture") != mat.values.end()) {
			material.baseColorTexture = getTexture(gltfModel.textures[mat.values["baseColorTexture"].TextureIndex()].source);
		}
		// Metallic roughness workflow
		if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
			material.metallicRoughnessTexture = getTexture(gltfModel.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source);
		}
		if (mat.values.find("roughnessFactor") != mat.values.end()) {
			material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
		}
		if (mat.values.find("metallicFactor") != mat.values.end()) {
			material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
		}
		if (mat.values.find("baseColorFactor") != mat.values.end()) {
			material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
		}
		if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
			material.normalTexture = getTexture(gltfModel.textures[mat.additionalValues["normalTexture"].TextureIndex()].source);
		}
		else {
			material.normalTexture = &emptyTexture;
		}
		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
			material.emissiveTexture = getTexture(gltfModel.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source);
		}
		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
			material.occlusionTexture = getTexture(gltfModel.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source);
		}
		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
			tinygltf::Parameter param = mat.additionalValues["alphaMode"];
			if (param.string_value == "BLEND") {
				material.alphaMode = Material::ALPHAMODE_BLEND;
			}
			if (param.string_value == "MASK") {
				material.alphaMode = Material::ALPHAMODE_MASK;
			}
		}
		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
		}

		materials.push_back(material);
	}
	// Push a default material at the end of the list for meshes with no material assigned
	materials.push_back(Material(pCoreBase));
}

void vkglTF::Model::loadAnimations(tinygltf::Model& gltfModel)
{
	for (tinygltf::Animation& anim : gltfModel.animations) {
		vkglTF::Animation animation{};
		animation.name = anim.name;
		if (anim.name.empty()) {
			//std::cout << "animation name:  " << animation.name << std::endl;
			animation.name = std::to_string(animations.size());
		}

		// Samplers
		for (auto& samp : anim.samplers) {
			vkglTF::AnimationSampler sampler{};

			if (samp.interpolation == "LINEAR") {
				sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
			}
			if (samp.interpolation == "STEP") {
				sampler.interpolation = AnimationSampler::InterpolationType::STEP;
			}
			if (samp.interpolation == "CUBICSPLINE") {
				sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
			}

			// Read sampler input time values
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[samp.input];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				float* buf = new float[accessor.count];
				memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(float));
				for (size_t index = 0; index < accessor.count; index++) {
					sampler.inputs.push_back(buf[index]);
				}
				delete[] buf;
				for (auto input : sampler.inputs) {
					if (input < animation.start) {
						animation.start = input;
					};
					if (input > animation.end) {
						animation.end = input;
					}
				}
			}

			// Read sampler output T/R/S values 
			{
				const tinygltf::Accessor& accessor = gltfModel.accessors[samp.output];
				const tinygltf::BufferView& bufferView = gltfModel.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = gltfModel.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				switch (accessor.type) {
				case TINYGLTF_TYPE_VEC3: {
					glm::vec3* buf = new glm::vec3[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::vec3));
					for (size_t index = 0; index < accessor.count; index++) {
						sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
					}
					delete[] buf;
					break;
				}
				case TINYGLTF_TYPE_VEC4: {
					glm::vec4* buf = new glm::vec4[accessor.count];
					memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::vec4));
					for (size_t index = 0; index < accessor.count; index++) {
						sampler.outputsVec4.push_back(buf[index]);
					}
					delete[] buf;
					break;
				}
				default: {
					//std::cout << "unknown type" << std::endl;
					break;
				}
				}
			}

			animation.samplers.push_back(sampler);
		}

		// Channels
		for (auto& source : anim.channels) {
			vkglTF::AnimationChannel channel{};

			if (source.target_path == "rotation") {
				channel.path = AnimationChannel::PathType::ROTATION;
			}
			if (source.target_path == "translation") {
				channel.path = AnimationChannel::PathType::TRANSLATION;
			}
			if (source.target_path == "scale") {
				channel.path = AnimationChannel::PathType::SCALE;
			}
			if (source.target_path == "weights") {
				//std::cout << "weights not yet supported, skipping channel" << std::endl;
				continue;
			}
			channel.samplerIndex = source.sampler;
			channel.node = nodeFromIndex(source.target_node);
			if (!channel.node) {
				continue;
			}

			animation.channels.push_back(channel);
		}

		animations.push_back(animation);
	}

	//updateAnimation(0, 1.0f);

}

void vkglTF::Model::loadFromFile(std::string filename, CoreBase* coreBase, VkQueue transferQueue, uint32_t fileLoadingFlags, float scale)
{

	//this->pCoreBase = coreBase;

	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;
	if (fileLoadingFlags & FileLoadingFlags::DontLoadImages) {
		gltfContext.SetImageLoader(vkgltf_loadImageDataFuncEmpty, nullptr);
	}
	else {
		gltfContext.SetImageLoader(vkgltf_loadImageDataFunc, nullptr);
	}
#if defined(__ANDROID__)
	// On Android all assets are packed with the apk in a compressed form, so we need to open them using the asset manager
	// We let tinygltf handle this, by passing the asset manager of our app
	tinygltf::asset_manager = androidApp->activity->assetManager;
#endif
	size_t pos = filename.find_last_of('/');
	path = filename.substr(0, pos);

	std::string error, warning;

	this->pCoreBase = coreBase;

#if defined(__ANDROID__)
	// On Android all assets are packed with the apk in a compressed form, so we need to open them using the asset manager
	// We let tinygltf handle this, by passing the asset manager of our app
	tinygltf::asset_manager = androidApp->activity->assetManager;
#endif
	bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);

	//std::vector<uint32_t> indexBuffer;
	//std::vector<Vertex> vertexBuffer;

	if (fileLoaded) {
		//if (!(fileLoadingFlags & FileLoadingFlags::DontLoadImages)) {
		loadImages(gltfModel, pCoreBase, transferQueue);
		//}
		loadMaterials(gltfModel);
		const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

		std::cout << "scene.nodes.size(): " << scene.nodes.size() << std::endl;

		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];
			loadNode(nullptr, node, scene.nodes[i], gltfModel, this->modelIndexBuffer, this->modelVertexBuffer, scale);
		}
		if (gltfModel.animations.size() > 0) {
			loadAnimations(gltfModel);
		}
		loadSkins(gltfModel);

		for (auto node : linearNodes) {
			// Assign skins
			if (node->skinIndex > -1) {
				node->skin = skins[node->skinIndex];
			}
			// Initial pose
			if (node->mesh) {
				node->update();
			}
		}
	}
	else {
		//vks::tools::exitFatal("Could not load glTF file \"" + filename + "\": " + error, -1);
		return;
	}

	// Pre-Calculations for requested features
	if ((fileLoadingFlags & FileLoadingFlags::PreTransformVertices) ||
		(fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors) ||
		(fileLoadingFlags & FileLoadingFlags::FlipY)) {
		const bool preTransform = fileLoadingFlags & FileLoadingFlags::PreTransformVertices;
		const bool preMultiplyColor = fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors;
		const bool flipY = fileLoadingFlags & FileLoadingFlags::FlipY;
		for (Node* node : linearNodes) {
			const glm::mat4 localMatrix = node->getMatrix();
			if (node->mesh) {
				for (Primitive* primitive : node->mesh->primitives) {
					for (uint32_t i = 0; i < primitive->vertexCount; i++) {
						Vertex& vertex = this->modelVertexBuffer[primitive->firstVertex + i];
						// Pre-transform vertex positions by node-hierarchy
						if (preTransform) {
							vertex.pos = localMatrix * glm::vec4(vertex.pos.x, vertex.pos.y, vertex.pos.z, 1.0f);
							vertex.normal = glm::normalize(localMatrix * glm::vec4(vertex.normal.x, vertex.normal.y, vertex.normal.z, 0.0f));
						}
						// Flip Y-Axis of vertex positions
						if (flipY) {
							vertex.pos.y *= -1.0f;
							vertex.normal.y *= -1.0f;
						}
						// Pre-Multiply vertex colors with material base color
						if (preMultiplyColor) {
							vertex.color = primitive->material.baseColorFactor * vertex.color;
						}
					}
				}
			}
		}
	}

	for (auto extension : gltfModel.extensionsUsed) {
		if (extension == "KHR_materials_pbrSpecularGlossiness") {
			//std::cout << "Required extension: " << extension;
			metallicRoughnessWorkflow = false;
		}
	}

	VkDeviceSize vertexBufferSize = this->modelVertexBuffer.size() * sizeof(Vertex);
	VkDeviceSize indexBufferSize = this->modelIndexBuffer.size() * sizeof(uint32_t);
	indices.count = static_cast<uint32_t>(this->modelIndexBuffer.size());
	vertices.count = static_cast<uint32_t>(this->modelVertexBuffer.size());

	assert((vertexBufferSize > 0) && (indexBufferSize > 0));

	//staging buffers
	vrt::Buffer vertexStaging{};
	vertexStaging.bufferData.bufferName = "_vertexStagingBuffer";
	vertexStaging.bufferData.bufferMemoryName = "_vertexStagingBufferMemory";

	vrt::Buffer indexStaging{};
	indexStaging.bufferData.bufferName = "_indexStagingBuffer";
	indexStaging.bufferData.bufferMemoryName = "_indexStagingBufferMemory";

	// Create staging buffers
	// Vertex data
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vertexStaging,
		vertexBufferSize,
		this->modelVertexBuffer.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel vertex staging buffer");
	}

	// Index data
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&indexStaging,
		indexBufferSize,
		this->modelIndexBuffer.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel index staging buffer");
	}

	// Create device local buffers
	//vertices buffer names
	vertices.buffer.bufferData.bufferName = "gltfModelVerticesBuffer";
	vertices.buffer.bufferData.bufferMemoryName = "gltfModelVerticesBufferMemory";

	//create vertices buffer
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&vertices.buffer,
		vertexBufferSize,
		nullptr) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel vertices buffer");
	}

	//indices buffer and memory names
	indices.buffer.bufferData.bufferName = "gltfModelIndicesBuffer";
	indices.buffer.bufferData.bufferMemoryName = "gltfModelIndicesBufferMemory";

	// Index buffer
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&indices.buffer,
		indexBufferSize,
		nullptr) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel indices buffer");
	}

	//copy from staging buffers
	//create temporary command buffer
	VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};

	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(cmdBuffer, vertexStaging.bufferData.buffer, vertices.buffer.bufferData.buffer, 1, &copyRegion);
	std::cout << "model load from file vertexBufferSize: " << vertexBufferSize << std::endl;
	VkBufferCopy copyRegion2{};
	copyRegion2.size = indexBufferSize;
	vkCmdCopyBuffer(cmdBuffer, indexStaging.bufferData.buffer, indices.buffer.bufferData.buffer, 1, &copyRegion2);

	//submit temporary command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//destroy staging buffers
	vkDestroyBuffer(pCoreBase->devices.logical, vertexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, vertexStaging.bufferData.memory, nullptr);
	vkDestroyBuffer(pCoreBase->devices.logical, indexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, indexStaging.bufferData.memory, nullptr);

	for (auto& verts : modelVertexBuffer) {
		tempVertexBuffer.push_back(verts);
	}

	getSceneDimensions();

	// Setup descriptors
	uint32_t uboCount{ 0 };
	uint32_t imageCount{ 0 };
	for (auto node : linearNodes) {
		if (node->mesh) {
			uboCount++;
		}
	}
	for (auto material : materials) {
		if (material.baseColorTexture != nullptr) {
			imageCount++;
		}
	}
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboCount },
	};
	if (imageCount > 0) {
		if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount });
		}
		if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount });
		}
	}
	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCI.pPoolSizes = poolSizes.data();
	descriptorPoolCI.maxSets = uboCount + imageCount;
	if (vkCreateDescriptorPool(pCoreBase->devices.logical, &descriptorPoolCI, nullptr, &descriptorPool));

	//// Descriptors for per-node uniform buffers
	//{
	//	// Layout is global, so only create if it hasn't already been created before
	//	if (descriptorSetLayoutUbo == VK_NULL_HANDLE) {
	//		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
	//			vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 0),
	//		};
	//		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
	//		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	//		descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	//		descriptorLayoutCI.pBindings = setLayoutBindings.data();
	//		if (vkCreateDescriptorSetLayout(pCoreBase->devices.logical, &descriptorLayoutCI, nullptr, &descriptorSetLayoutUbo));
	//	}
	//	for (auto node : nodes) {
	//		prepareNodeDescriptor(node, descriptorSetLayoutUbo);
	//	}
	//}
	//
	//// Descriptors for per-material images
	//{
	//	// Layout is global, so only create if it hasn't already been created before
	//	if (descriptorSetLayoutImage == VK_NULL_HANDLE) {
	//		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
	//		if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
	//			setLayoutBindings.push_back(vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(setLayoutBindings.size())));
	//		}
	//		if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
	//			setLayoutBindings.push_back(vks::initializers::descriptorSetLayoutBinding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, static_cast<uint32_t>(setLayoutBindings.size())));
	//		}
	//		VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
	//		descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	//		descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
	//		descriptorLayoutCI.pBindings = setLayoutBindings.data();
	//		if (vkCreateDescriptorSetLayout(pCoreBase->devices.logical, &descriptorLayoutCI, nullptr, &descriptorSetLayoutImage));
	//	}
	//	for (auto& material : materials) {
	//		if (material.baseColorTexture != nullptr) {
	//			material.createDescriptorSet(descriptorPool, vkglTF::descriptorSetLayoutImage, descriptorBindingFlags);
	//		}
	//	}
	//}
	}

//void vkglTF::Model::bindBuffers(VkCommandBuffer commandBuffer)
//{
//	const VkDeviceSize offsets[1] = { 0 };
//	vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
//	vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
//	buffersBound = true;
//}

//void vkglTF::Model::drawNode(Node* node, VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet)
//{
//	if (node->mesh) {
//		for (Primitive* primitive : node->mesh->primitives) {
//			bool skip = false;
//			const vkglTF::Material& material = primitive->material;
//			if (renderFlags & RenderFlags::RenderOpaqueNodes) {
//				skip = (material.alphaMode != Material::ALPHAMODE_OPAQUE);
//			}
//			if (renderFlags & RenderFlags::RenderAlphaMaskedNodes) {
//				skip = (material.alphaMode != Material::ALPHAMODE_MASK);
//			}
//			if (renderFlags & RenderFlags::RenderAlphaBlendedNodes) {
//				skip = (material.alphaMode != Material::ALPHAMODE_BLEND);
//			}
//			if (!skip) {
//				if (renderFlags & RenderFlags::BindImages) {
//					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, bindImageSet, 1, &material.descriptorSet, 0, nullptr);
//				}
//				vkCmdDrawIndexed(commandBuffer, primitive->indexCount, 1, primitive->firstIndex, 0, 0);
//			}
//		}
//	}
//	for (auto& child : node->children) {
//		drawNode(child, commandBuffer, renderFlags, pipelineLayout, bindImageSet);
//	}
//}

//void vkglTF::Model::draw(VkCommandBuffer commandBuffer, uint32_t renderFlags, VkPipelineLayout pipelineLayout, uint32_t bindImageSet)
//{
//	if (!buffersBound) {
//		const VkDeviceSize offsets[1] = { 0 };
//		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertices.buffer, offsets);
//		vkCmdBindIndexBuffer(commandBuffer, indices.buffer, 0, VK_INDEX_TYPE_UINT32);
//	}
//	for (auto& node : nodes) {
//		drawNode(node, commandBuffer, renderFlags, pipelineLayout, bindImageSet);
//	}
//}

void vkglTF::Model::getNodeDimensions(Node* node, glm::vec3& min, glm::vec3& max)
{
	if (node->mesh) {
		for (Primitive* primitive : node->mesh->primitives) {
			glm::vec4 locMin = glm::vec4(primitive->dimensions.min, 1.0f) * node->getMatrix();
			glm::vec4 locMax = glm::vec4(primitive->dimensions.max, 1.0f) * node->getMatrix();
			if (locMin.x < min.x) { min.x = locMin.x; }
			if (locMin.y < min.y) { min.y = locMin.y; }
			if (locMin.z < min.z) { min.z = locMin.z; }
			if (locMax.x > max.x) { max.x = locMax.x; }
			if (locMax.y > max.y) { max.y = locMax.y; }
			if (locMax.z > max.z) { max.z = locMax.z; }
		}
	}
	for (auto child : node->children) {
		getNodeDimensions(child, min, max);
	}
}

void vkglTF::Model::getSceneDimensions()
{
	dimensions.min = glm::vec3(FLT_MAX);
	dimensions.max = glm::vec3(-FLT_MAX);
	for (auto node : nodes) {
		getNodeDimensions(node, dimensions.min, dimensions.max);
	}
	dimensions.size = dimensions.max - dimensions.min;
	dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
	dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
}

void vkglTF::Model::updateAnimation(uint32_t index, float time) {

	if (index > static_cast<uint32_t>(animations.size()) - 1) {
		//std::cout << "No animation with index " << index << std::endl;
		return;
	}


	//Animation& animation = animations[index];

	////std::cout << "animation.end:  " << animation.end << std::endl;

	animations[index].currentTime += time;

	if (animations[index].currentTime >= animations[index].end)
	{
		animations[index].currentTime -= animations[index].end;
	}

	bool updated = false;

	//int idx = 0;

	for (auto& channel : animations[index].channels) {
		////std::cout << "channel.node->name " << channel.node->name << std::endl;
		vkglTF::AnimationSampler& sampler = animations[index].samplers[channel.samplerIndex];

		if (sampler.inputs.size() > sampler.outputsVec4.size()) {
			continue;
		}

		for (auto i = 0; i < sampler.inputs.size() - 1; i++) {
			if ((animations[index].currentTime >= sampler.inputs[i]) && (animations[index].currentTime <= sampler.inputs[i + 1])) {
				float u = std::max(0.0f, animations[index].currentTime - sampler.inputs[i]) / (sampler.inputs[static_cast<std::vector<float, std::allocator<float>>::size_type>(i) + 1] - sampler.inputs[i]);
				if (u <= 1.0f) {
					switch (channel.path) {
					case vkglTF::AnimationChannel::PathType::TRANSLATION: {
						glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
						channel.node->translation = glm::vec3(trans);
						break;
					}
					case vkglTF::AnimationChannel::PathType::SCALE: {
						glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
						channel.node->scale = glm::vec3(trans);
						break;
					}
					case vkglTF::AnimationChannel::PathType::ROTATION: {
						glm::quat q1;
						q1.x = sampler.outputsVec4[i].x;
						q1.y = sampler.outputsVec4[i].y;
						q1.z = sampler.outputsVec4[i].z;
						q1.w = sampler.outputsVec4[i].w;
						glm::quat q2;
						q2.x = sampler.outputsVec4[i + 1].x;
						q2.y = sampler.outputsVec4[i + 1].y;
						q2.z = sampler.outputsVec4[i + 1].z;
						q2.w = sampler.outputsVec4[i + 1].w;
						channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
						break;
					}
					}
					updated = true;
					//++idx;
					////std::cout << "updated idx" << idx << std::endl;

					// Output the updated transformation values
					////std::cout << "Node Name: " << channel.node->name << std::endl;
					//
					//	//channel.node->getMatrix();
					//	//std::cout << "channel.node->matrix:\n";
					//	for (int i = 0; i < 4; ++i) {
					//		for (int j = 0; j < 4; ++j) {
					//			//std::cout << channel.node->getMatrix()[i][j] << " ";
					//		}
					//		//std::cout << std::endl;
					//	}
					////std::cout << "Translation: " << "(" << channel.node->translation.x << ", " << channel.node->translation.y << ", " << channel.node->translation.z << ")" << std::endl;
					////std::cout << "Scale: " << "(" << channel.node->scale.x << ", " << channel.node->scale.y << ", " << channel.node->scale.z << ")" << std::endl;
					//glm::vec3 euler = glm::degrees(glm::eulerAngles(channel.node->rotation));
					////std::cout << "Rotation: " << "(" << euler.x << ", " << euler.y << ", " << euler.z << ")" << std::endl;
				}
			}
		}
	}
	////std::cout << "updated: " << updated << std::endl;
	if (updated) {
		for (auto& node : linearNodes) {
			node->update();
		}

	}

}

/*
	Helper functions
*/
vkglTF::Node* vkglTF::Model::findNode(Node* parent, uint32_t index) {
	Node* nodeFound = nullptr;
	if (parent->index == index) {
		return parent;
	}
	for (auto& child : parent->children) {
		nodeFound = findNode(child, index);
		if (nodeFound) {
			break;
		}
	}
	return nodeFound;
}

vkglTF::Node* vkglTF::Model::nodeFromIndex(uint32_t index) {
	Node* nodeFound = nullptr;
	for (auto& node : nodes) {
		nodeFound = findNode(node, index);
		if (nodeFound) {
			break;
		}
	}
	return nodeFound;
}

void vkglTF::Model::DestroyModel() {
	vkDestroyBuffer(pCoreBase->devices.logical, vertices.buffer.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, vertices.buffer.bufferData.memory, nullptr);
	vkDestroyBuffer(pCoreBase->devices.logical, indices.buffer.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, indices.buffer.bufferData.memory, nullptr);
	for (auto texture : textures) {
		texture.destroy();
	}
	for (auto node : nodes) {
		delete node;
	}
	for (auto skin : skins) {
		delete skin;
	}
	if (descriptorSetLayoutUbo != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(pCoreBase->devices.logical, descriptorSetLayoutUbo, nullptr);
		descriptorSetLayoutUbo = VK_NULL_HANDLE;
	}
	if (descriptorSetLayoutImage != VK_NULL_HANDLE) {
		vkDestroyDescriptorSetLayout(pCoreBase->devices.logical, descriptorSetLayoutImage, nullptr);
		descriptorSetLayoutImage = VK_NULL_HANDLE;
	}
	vkDestroyDescriptorPool(pCoreBase->devices.logical, descriptorPool, nullptr);
	emptyTexture.destroy();
}

//void vkglTF::Model::prepareNodeDescriptor(vkglTF::Node* node, VkDescriptorSetLayout descriptorSetLayout) {
//	if (node->mesh) {
//		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
//		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//		descriptorSetAllocInfo.descriptorPool = descriptorPool;
//		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
//		descriptorSetAllocInfo.descriptorSetCount = 1;
//		if (vkAllocateDescriptorSets(pCoreBase->devices.logical, &descriptorSetAllocInfo, &node->mesh->uniformBuffer.buffer.bufferData.descriptorSet));
//
//		VkWriteDescriptorSet writeDescriptorSet{};
//		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//		writeDescriptorSet.descriptorCount = 1;
//		writeDescriptorSet.dstSet = node->mesh->uniformBuffer.buffer.bufferData.descriptorSet;
//		writeDescriptorSet.dstBinding = 0;
//		writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.buffer.bufferData.descriptor;
//
//		vkUpdateDescriptorSets(pCoreBase->devices.logical, 1, &writeDescriptorSet, 0, nullptr);
//	}
//	for (auto& child : node->children) {
//		prepareNodeDescriptor(child, descriptorSetLayout);
//	}
//}
