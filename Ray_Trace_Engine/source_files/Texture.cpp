#include "Texture.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

namespace vrt {

	Texture::Texture() {

	}

	Texture::Texture(CoreBase* coreBase) {
		InitTexture(coreBase);
		//loadFromFile("C:/Users/akral/projects/Ray_Trace_Engine/Ray_Trace_Engine/assets/textures/gratefloor_rgba.ktx",
		//	VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_SAMPLED_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}

	void Texture::InitTexture(CoreBase* coreBase) {
		this->pCoreBase = coreBase;
	}

	void Texture::updateDescriptor() {
		descriptor.sampler = sampler;
		descriptor.imageView = view;
		descriptor.imageLayout = imageLayout;
	}

	ktxResult Texture::loadKTXFile(std::string filename, ktxTexture** target) {
		ktxResult result = KTX_SUCCESS;

		result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, target);

		return result;
	}

	void Texture::loadFromFile(std::string filename, VkFormat format, VkImageUsageFlags imageUsageFlags, VkImageLayout imageLayout, bool forceLinear) {

		//ktx texture pointer
		ktxTexture* ktxTexture;

		std::cout << "\nfile name: " << filename << std::endl;

		// Extract the filename using std::filesystem
		std::filesystem::path filePath(filename);
		std::string extractedFilename = "_" + filePath.filename().string();

		// Output the extracted filename
		std::cout << "\nExtracted filename: " << extractedFilename << "\n" << std::endl;

		//load ktx file
		ktxResult result = loadKTXFile(filename, &ktxTexture);
		assert(result == KTX_SUCCESS);

		//ktx file dimensions and mip properties
		width = ktxTexture->baseWidth;
		height = ktxTexture->baseHeight;
		mipLevels = ktxTexture->numLevels;

		//output data for test
		std::cout << "Width: " << ktxTexture->baseWidth << std::endl;
		std::cout << "Height: " << ktxTexture->baseHeight << std::endl;
		std::cout << "Number of Mip Levels: " << ktxTexture->numLevels << std::endl;

		//get ktx file size properties
		ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
		ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(ktxTexture);

		//output data for test
		std::cout << "\n ktx texture size: " << ktxTextureSize << std::endl;

		//get device properties for the requested texture format
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(pCoreBase->devices.physical, format, &formatProperties);

		// Only use linear tiling if requested (and supported by the device)
		// Support for linear tiling is mostly limited, so prefer to use
		// optimal tiling instead
		// On most implementations linear tiling will only support a very
		// limited amount of formats and features (mip maps, cubemaps, arrays, etc.)
		VkBool32 useStaging = !forceLinear;

		//buffer memory allocate info
		VkMemoryAllocateInfo bufferMemoryAllocateInfo{};
		bufferMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

		//buffer memory requirements
		VkMemoryRequirements bufferMemoryRequirements;

		// Use a separate command buffer for texture loading
		VkCommandBuffer copyCmd = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

		if (useStaging) {

			std::cout << "using staging to create image -- optimal tiling " << std::endl;

			//create host visible staging buffer
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			//buffer create info
			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = ktxTextureSize;

			//buffer used as a transfer source for the buffer copy
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			//create staging buffer
			//add handle id and name to map
			pCoreBase->add([this, &bufferCreateInfo, &stagingBuffer]()
				{return pCoreBase->objCreate.VKCreateBuffer(&bufferCreateInfo,
					nullptr, &stagingBuffer);}, "ktxLoadImageStagingBuffer" + extractedFilename);

			//get memory requirements for the staging buffer (alignment, memory type bits)
			vkGetBufferMemoryRequirements(pCoreBase->devices.logical, stagingBuffer, &bufferMemoryRequirements);
			bufferMemoryAllocateInfo.allocationSize = bufferMemoryRequirements.size;

			//get memory type index for a host visible buffer
			bufferMemoryAllocateInfo.memoryTypeIndex = pCoreBase->getMemoryType(bufferMemoryRequirements.memoryTypeBits,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			//allocate staging buffer memory
			//add handle and id to map
			pCoreBase->add([this, &bufferMemoryAllocateInfo, &stagingMemory]()
				{return pCoreBase->objCreate.VKAllocateMemory(&bufferMemoryAllocateInfo,
					nullptr, &stagingMemory);}, "ktxLoadImageStagingBufferMemory" + extractedFilename);

			//bind staging buffer and memory
			if (vkBindBufferMemory(pCoreBase->devices.logical, stagingBuffer, stagingMemory, 0) != VK_SUCCESS) {
				throw std::invalid_argument("failed to ");
			}

			//data pointer that will hold image data
			uint8_t* data;

			//map memory
			if (vkMapMemory(pCoreBase->devices.logical, stagingMemory, 0, bufferMemoryRequirements.size, 0, (void**)&data) != VK_SUCCESS) {
				throw std::invalid_argument("failed to ");
			}

			//copy texture data into staging buffer
			memcpy(data, ktxTextureData, ktxTextureSize);

			//unmap memory
			//vkUnmapMemory(pCoreBase->devices.logical, stagingMemory);

			// Setup buffer copy regions for each mip level
			std::vector<VkBufferImageCopy> bufferCopyRegions;

			//generate mips
			for (uint32_t i = 0; i < mipLevels; i++) {
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

			//create optimal tiled target image
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
			imageCreateInfo.usage = imageUsageFlags;

			// Ensure that the TRANSFER_DST bit is set for staging
			if (!(imageCreateInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
				imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
			}

			//create image
			//add handle and name to map
			pCoreBase->add([this, &imageCreateInfo]()
				{return pCoreBase->objCreate.VKCreateImage(&imageCreateInfo,
					nullptr, &image);}, "ktxTextureImage" + extractedFilename);

			//image memory allocate info
			VkMemoryAllocateInfo imageMemoryAllocateInfo{};
			imageMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

			//image memory requirements
			VkMemoryRequirements imageMemoryRequirements;

			//get image memory requirements
			vkGetImageMemoryRequirements(pCoreBase->devices.logical, image, &imageMemoryRequirements);

			//update memory allocate info
			imageMemoryAllocateInfo.allocationSize = imageMemoryRequirements.size;
			imageMemoryAllocateInfo.memoryTypeIndex = pCoreBase->getMemoryType(imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			//allocate image memory
			//add handle and name to map
			//if (vkAllocateMemory(pCoreBase->devices.logical, &imageMemoryAllocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
			//	throw std::invalid_argument("failed to ");
			//}

			pCoreBase->add([this, &imageMemoryAllocateInfo]()
				{return pCoreBase->objCreate.VKAllocateMemory(&imageMemoryAllocateInfo,
					nullptr, &imageMemory);}, "ktxTextureImageMemory" + extractedFilename);

			if (vkBindImageMemory(pCoreBase->devices.logical, image, imageMemory, 0) != VK_SUCCESS) {
				throw std::invalid_argument("failed to ");
			}

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = mipLevels;
			subresourceRange.layerCount = 1;

			// Image barrier for optimal image (target)
			// Optimal image will be used as destination for the copy
			vrt::Tools::setImageLayout(
				copyCmd,
				image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				subresourceRange);

			// Copy mip levels from staging buffer
			vkCmdCopyBufferToImage(
				copyCmd,
				stagingBuffer,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				static_cast<uint32_t>(bufferCopyRegions.size()),
				bufferCopyRegions.data()
			);

			// Change texture image layout to shader read after all mip levels have been copied
			this->imageLayout = imageLayout;
			vrt::Tools::setImageLayout(
				copyCmd,
				image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				imageLayout,
				subresourceRange);

			pCoreBase->FlushCommandBuffer(copyCmd, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

			// Clean up staging resources
			vkFreeMemory(pCoreBase->devices.logical, stagingMemory, nullptr);
			vkDestroyBuffer(pCoreBase->devices.logical, stagingBuffer, nullptr);
		}

		else {

			std::cout << "NOT using staging to create image -- linear tiling " << std::endl;

			// Prefer using optimal tiling, as linear tiling 
			// may support only a small set of features 
			// depending on implementation (e.g. no mip maps, only one layer, etc.)

			//check if this support is supported for linear tiling
			assert(formatProperties.linearTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);

			//mappable image and memory decl.
			VkImage mappableImage;
			VkDeviceMemory mappableMemory;

			//mappable image create info
			VkImageCreateInfo imageCreateInfo{};
			imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
			imageCreateInfo.format = format;
			imageCreateInfo.extent = { width, height, 1 };
			imageCreateInfo.mipLevels = 1;
			imageCreateInfo.arrayLayers = 1;
			imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			imageCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
			imageCreateInfo.usage = imageUsageFlags;
			imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
			imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

			//create mappable image
			pCoreBase->add([this, &imageCreateInfo, &mappableImage]()
				{return pCoreBase->objCreate.VKCreateImage(&imageCreateInfo,
					nullptr, &mappableImage);}, "ktxTextureImageMappable" + extractedFilename);

			//image memory allocate info
			VkMemoryAllocateInfo imageMemoryAllocateInfo{};
			imageMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

			//buffer memory requirements
			VkMemoryRequirements imageMemoryRequirements;

			//get memory requirements for this image
			vkGetImageMemoryRequirements(pCoreBase->devices.logical, mappableImage, &imageMemoryRequirements);

			//set memory allocation size to required memory size
			imageMemoryAllocateInfo.allocationSize = imageMemoryRequirements.size;

			//get memory type that can be mapped to host memory
			imageMemoryAllocateInfo.memoryTypeIndex = pCoreBase->getMemoryType(
				imageMemoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			//allocate mappable image memory
			//add handle id and name to map
			pCoreBase->add([this, &imageMemoryAllocateInfo, &mappableMemory]()
				{return pCoreBase->objCreate.VKAllocateMemory(&imageMemoryAllocateInfo,
					nullptr, &mappableMemory);}, "ktxTextureImageMemoryMappable" + extractedFilename);


			// Bind allocated image for use
			if (vkBindImageMemory(pCoreBase->devices.logical, mappableImage, mappableMemory, 0) != VK_SUCCESS) {
				throw std::invalid_argument("failed to ");
			}

			// Get sub resource layout
			// Mip map count, array layer, etc.
			VkImageSubresource subRes = {};
			subRes.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subRes.mipLevel = 0;

			VkSubresourceLayout subResLayout;
			void* data;

			// Get sub resources layout 
			// Includes row pitch, size offsets, etc.
			vkGetImageSubresourceLayout(pCoreBase->devices.logical, mappableImage, &subRes, &subResLayout);

			//map image memory
			if (vkMapMemory(pCoreBase->devices.logical, mappableMemory, 0, imageMemoryRequirements.size, 0, &data) != VK_SUCCESS) {
				throw std::invalid_argument("failed to ");
			}

			//copy image data into memory
			memcpy(data, ktxTextureData, imageMemoryRequirements.size);

			//unmap image memory
			vkUnmapMemory(pCoreBase->devices.logical, mappableMemory);

			// Linear tiled images don't need to be staged
			// and can be directly used as textures
			image = mappableImage;
			imageMemory = mappableMemory;
			this->imageLayout = imageLayout;

			//subresource range
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			//set image layout
			this->imageLayout = imageLayout;
			vrt::Tools::setImageLayout(
				copyCmd,
				image,
				VK_IMAGE_LAYOUT_UNDEFINED,
				imageLayout,
				subresourceRange);

			//end, submit, and free allocated command buffer
			pCoreBase->FlushCommandBuffer(copyCmd, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);
		}

		ktxTexture_Destroy(ktxTexture);

		//create default sampler
		VkSamplerCreateInfo samplerCreateInfo = {};
		samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
		samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerCreateInfo.mipLodBias = 0.0f;
		samplerCreateInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerCreateInfo.minLod = 0.0f;

		//max level-of-detail should match mip level count
		samplerCreateInfo.maxLod = (useStaging) ? (float)mipLevels : 0.0f;

		//only enable anisotropic filtering if enabled on the device
		samplerCreateInfo.maxAnisotropy = pCoreBase->deviceData.features.samplerAnisotropy ? pCoreBase->deviceProperties.physicalDevice.limits.maxSamplerAnisotropy : 1.0f;
		samplerCreateInfo.anisotropyEnable = pCoreBase->deviceData.features.samplerAnisotropy;
		samplerCreateInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

		//create sampler
		//adds sampler handle id and name to map
		pCoreBase->add([this, &samplerCreateInfo]()
			{return pCoreBase->objCreate.VKCreateSampler(&samplerCreateInfo,
				nullptr, &sampler);}, "ktxTextureImageSampler" + extractedFilename);


		//imageview create info
		VkImageViewCreateInfo viewCreateInfo = {};
		viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewCreateInfo.format = format;
		viewCreateInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
		viewCreateInfo.subresourceRange.levelCount = (useStaging) ? mipLevels : 1;
		viewCreateInfo.image = image;

		//create image view
		//adds imageview handle id and name to map
		pCoreBase->add([this, &viewCreateInfo]()
			{return pCoreBase->objCreate.VKCreateImageView(&viewCreateInfo,
				nullptr, &view);}, "ktxTextureImageView" + extractedFilename);

		// Update descriptor image info member that can be used for setting up descriptor sets
		updateDescriptor();
	}

	void Texture::DestroyTexture() {
		if (this->pCoreBase) {
			vkDestroyImageView(pCoreBase->devices.logical, view, nullptr);
			vkDestroyImage(pCoreBase->devices.logical, image, nullptr);
			vkFreeMemory(pCoreBase->devices.logical, imageMemory, nullptr);
			vkDestroySampler(pCoreBase->devices.logical, sampler, nullptr);
		}
	}

	void Texture::fromglTfImage(tinygltf::Image& gltfimage, std::string path, CoreBase* coreBase, VkQueue copyQueue) {

		bool isKtx = false;

		// Image points to an external ktx file
		if (gltfimage.uri.find_last_of(".") != std::string::npos) {
			if (gltfimage.uri.substr(gltfimage.uri.find_last_of(".") + 1) == "ktx") {
				isKtx = true;
			}
		}


		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

		//texture was loaded using STB_Image
		if (!isKtx) {
		//std::cout << "gltfimage.uri: " << gltfimage.uri << std::endl;

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

			//format = VK_FORMAT_R8G8B8A8_UNORM;

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

			if (vkCreateBuffer(pCoreBase->devices.logical, &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create staging buffer");
			}

			vkGetBufferMemoryRequirements(pCoreBase->devices.logical, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
				throw std::invalid_argument("failed to allocate staging buffer memory");
			}

			if (vkBindBufferMemory(pCoreBase->devices.logical, stagingBuffer, stagingMemory, 0) != VK_SUCCESS) {
				throw std::invalid_argument("failed to bind staging buffer and memory");
			}

			uint8_t* data;

			if (vkMapMemory(pCoreBase->devices.logical, stagingMemory, 0, memReqs.size, 0, (void**)&data) != VK_SUCCESS) {
				throw std::invalid_argument("failed to map staging buffer memory");
			}

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

			if (vkCreateImage(pCoreBase->devices.logical, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create texture image");
			}

			vkGetImageMemoryRequirements(pCoreBase->devices.logical, image, &memReqs);

			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
				throw std::invalid_argument("failed to allocate texture image memory");
			}

			if (vkBindImageMemory(pCoreBase->devices.logical, image, imageMemory, 0) != VK_SUCCESS) {
				throw std::invalid_argument("failed to bind texture image and memory");
			}

			VkCommandBuffer copyCmd = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = 0;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			VkBufferImageCopy bufferCopyRegion = {};
			bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			bufferCopyRegion.imageSubresource.mipLevel = 0;
			bufferCopyRegion.imageSubresource.baseArrayLayer = 0;
			bufferCopyRegion.imageSubresource.layerCount = 1;
			bufferCopyRegion.imageExtent.width = width;
			bufferCopyRegion.imageExtent.height = height;
			bufferCopyRegion.imageExtent.depth = 1;

			vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufferCopyRegion);

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(copyCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			pCoreBase->FlushCommandBuffer(copyCmd, copyQueue, pCoreBase->commandPools.graphics, true);

			vkFreeMemory(pCoreBase->devices.logical, stagingMemory, nullptr);
			vkDestroyBuffer(pCoreBase->devices.logical, stagingBuffer, nullptr);

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

			{
				VkImageMemoryBarrier imageMemoryBarrier{};
				imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
				imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
				imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				imageMemoryBarrier.image = image;
				imageMemoryBarrier.subresourceRange = subresourceRange;
				vkCmdPipelineBarrier(blitCmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
			}

			if (deleteBuffer) {
				delete[] buffer;
			}

			pCoreBase->FlushCommandBuffer(blitCmd, copyQueue, pCoreBase->commandPools.graphics, true);

		}



		else {
			// Texture is stored in an external ktx file
			std::string filename = path + "/" + gltfimage.uri;

			std::cout << "Texture loading .KTX\ngltfimage.uri: " << gltfimage.uri << std::endl;

			//ktl texture
			ktxTexture* ktxTexture;

			ktxResult result = KTX_SUCCESS;

			//creates ktl texture
			result = ktxTexture_CreateFromNamedFile(filename.c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);

			assert(result == KTX_SUCCESS);

			//texture dimensions
			width = ktxTexture->baseWidth;
			height = ktxTexture->baseHeight;
			mipLevels = ktxTexture->numLevels;

			//texture buffer data and size
			ktx_uint8_t* ktxTextureData = ktxTexture_GetData(ktxTexture);
			ktx_size_t ktxTextureSize = ktxTexture_GetDataSize(ktxTexture);

			//texture format
			format = VK_FORMAT_R8G8B8A8_UNORM;

			//device properties for the requested texture format
			VkFormatProperties formatProperties;
			vkGetPhysicalDeviceFormatProperties(pCoreBase->devices.physical, format, &formatProperties);

			//command buffer create data for memory copy
			VkCommandBuffer copyCmd = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

			//staging buffer and memory
			VkBuffer stagingBuffer;
			VkDeviceMemory stagingMemory;

			//staging buffer create info
			VkBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferCreateInfo.size = ktxTextureSize;

			// This buffer is used as a transfer source for the buffer copy
			bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
			bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			//create staging buffer
			if (vkCreateBuffer(pCoreBase->devices.logical, &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create gltx texture staging buffer");
			}

			//memory allocate info
			VkMemoryAllocateInfo memAllocInfo{};
			memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;

			//staging buffer memory requirements
			VkMemoryRequirements memReqs;
			vkGetBufferMemoryRequirements(pCoreBase->devices.logical, stagingBuffer, &memReqs);
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			//allocate staging buffer memory
			if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
				throw std::invalid_argument("failed to allocate gltx texture staging buffer memory");
			}

			//bind staging buffer and memory
			if (vkBindBufferMemory(pCoreBase->devices.logical, stagingBuffer, stagingMemory, 0) != VK_SUCCESS) {
				throw std::invalid_argument("failed to bind gltx texture staging buffer and memory");
			}

			//pointer to hold image data
			uint8_t* data;

			//map buffer memory
			if (vkMapMemory(pCoreBase->devices.logical, stagingMemory, 0, memReqs.size, 0, (void**)&data) != VK_SUCCESS) {
				throw std::invalid_argument("failed to bind gltx texture staging buffer and memory");
			}

			//copy image data to buffer
			memcpy(data, ktxTextureData, ktxTextureSize);

			//unmap 
			vkUnmapMemory(pCoreBase->devices.logical, stagingMemory);

			std::vector<VkBufferImageCopy> bufferCopyRegions;

			for (uint32_t i = 0; i < mipLevels; i++) {
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

			//optimal tiled target image create info
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

			//create optimal tiled target image
			if (vkCreateImage(pCoreBase->devices.logical, &imageCreateInfo, nullptr, &image) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create gltx texture image");
			}

				vkGetImageMemoryRequirements(pCoreBase->devices.logical, image, &memReqs);
				memAllocInfo.allocationSize = memReqs.size;
				memAllocInfo.memoryTypeIndex = pCoreBase->getMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
				if (vkAllocateMemory(pCoreBase->devices.logical, &memAllocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
					throw std::invalid_argument("failed to allocate gltx texture image memory");
				}

				if (vkBindImageMemory(pCoreBase->devices.logical, image, imageMemory, 0) != VK_SUCCESS) {
					throw std::invalid_argument("failed to bind gltx texture image and memory");
				}
			
				VkImageSubresourceRange subresourceRange = {};
				subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				subresourceRange.baseMipLevel = 0;
				subresourceRange.levelCount = mipLevels;
				subresourceRange.layerCount = 1;
			
				vrt::Tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);

				vkCmdCopyBufferToImage(copyCmd, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					static_cast<uint32_t>(bufferCopyRegions.size()), bufferCopyRegions.data());

				vrt::Tools::setImageLayout(copyCmd, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);

				pCoreBase->FlushCommandBuffer(copyCmd, copyQueue, this->pCoreBase->commandPools.graphics, true);

				this->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			
				//vkFreeMemory(pCoreBase->devices.logical, stagingMemory, nullptr);
				//vkDestroyBuffer(pCoreBase->devices.logical, stagingBuffer, nullptr);
			
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
		if (vkCreateSampler(pCoreBase->devices.logical, &samplerInfo, nullptr, &sampler) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create gltx texture sampler");
		}

		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.layerCount = 1;
		viewInfo.subresourceRange.levelCount = mipLevels;
		if (vkCreateImageView(pCoreBase->devices.logical, &viewInfo, nullptr, &view) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create gltx image view");

			descriptor.sampler = sampler;
			descriptor.imageView = view;
			descriptor.imageLayout = imageLayout;
		}
	}
}