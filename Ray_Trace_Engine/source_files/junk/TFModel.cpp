#include "TFModel.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>

//We use a custom image loading function with tinyglTF, so we can do custom stuff loading ktx textures
bool loadImageDataFunc(tinygltf::Image* image, const int imageIndex, std::string* error, std::string* warning,
	int req_width, int req_height, const unsigned char* bytes, int size, void* userData) {
	// KTX files will be handled by our own code
	if (image->uri.find_last_of(".") != std::string::npos) {
		if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx") {
			return true;
		}
	}

	return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);
}

bool loadImageDataFuncEmpty(tinygltf::Image* image, const int imageIndex, std::string* error,
	std::string* warning, int req_width, int req_height, const unsigned char* bytes, int size, void* userData) {
	// This function will be used for samples that don't require images to be loaded
	return true;
}

TFModel::TFModel() { }

TFModel::TFModel(CoreBase* coreBase, std::string filePath) {
	InitTFModel(coreBase, filePath);
}

void TFModel::InitTFModel(CoreBase* coreBase, std::string filePath) {

	//init core pointer
	this->pCoreBase = coreBase;


	//locally set loading flags
	uint32_t local_gltfLoadingFlags =
		static_cast<uint64_t>(FileLoadingFlags::PreTransformVertices) |
		static_cast<uint64_t>(FileLoadingFlags::PreMultiplyVertexColors) |
		static_cast<uint64_t>(FileLoadingFlags::FlipY);

	LoadFromFile(filePath, coreBase, coreBase->queue.graphics, local_gltfLoadingFlags, 1.0f);
}

void TFModel::LoadFromFile(std::string filename, CoreBase* coreBase, VkQueue transferQueue, uint32_t fileLoadingFlags, float scale) {

	//std::cout << "\nTFModel\n''''''''''''''''\nfilename: " << filename << "\n" << std::endl;

	//tinygltf model and context
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;

	//set image loader per file loading flags
	if (fileLoadingFlags & static_cast<uint64_t>(FileLoadingFlags::DontLoadImages)) {
		gltfContext.SetImageLoader(loadImageDataFuncEmpty, nullptr);
	}

	else {
		gltfContext.SetImageLoader(loadImageDataFunc, nullptr);
	}

	//filepath
	size_t pos = filename.find_last_of('/');
	path = filename.substr(0, pos);

	std::string error;
	std::string warning;

	//load model file
	bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);

	////vectors for indices and vertices
	//std::vector<uint32_t> indexBuffer;
	//std::vector<Vertex> vertexBuffer;

	//load images if model file loaded
	if (fileLoaded) {
		if (!(fileLoadingFlags & static_cast<uint32_t>(FileLoadingFlags::DontLoadImages))) {
			//std::cout << "loading images" << std::endl;
			LoadImages(gltfModel, pCoreBase->queue.graphics);
		}

		//load materials
		LoadMaterials(gltfModel);

		const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

		//load nodes
		for (size_t i = 0; i < scene.nodes.size(); i++) {

			//std::cout << "TFModel Load Node Loop Iteration [" << i << "]" << std::endl;

			const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];

			LoadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);

		}

		//load animations
		if (gltfModel.animations.size() > 0) {
			LoadAnimations(gltfModel);
		}

		//load skins/skinned meshes
		LoadSkins(gltfModel);

		if (skins.size() == 0) {

			skins.resize(1);
			skins[0].inverseBindMatrices.push_back(glm::mat4(1.0f));

			if (this->pCoreBase->CreateBuffer(
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&skins[0].ssbo,
				sizeof(glm::mat4) * skins[0].inverseBindMatrices.size(),
				skins[0].inverseBindMatrices.data()) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create skin ssbo");
			}
		}
		//assign skins to nodes if they exist
		for (auto& node : linearNodes) {

			// Assign skins
			if (node.skinIndex >= 0) {
				node.skin = &skins[node.skinIndex];
			}

			// Initial pose
			if (node.hasMesh) {
				node.Update();
			}
		}
	}

	//if model file fails to load
	else {
		//std::cout << "sorry sir and or madame, but the gltf file has failed to load." << std::endl;
		return;
	}

	// Pre-Calculations for requested features
	if ((fileLoadingFlags & static_cast<uint32_t>(FileLoadingFlags::PreTransformVertices)) ||
		(fileLoadingFlags & static_cast<uint32_t>(FileLoadingFlags::PreMultiplyVertexColors)) ||
		(fileLoadingFlags & static_cast<uint32_t>(FileLoadingFlags::FlipY))) {

		const bool preTransform = fileLoadingFlags & static_cast<uint32_t>(FileLoadingFlags::PreTransformVertices);
		const bool preMultiplyColor = fileLoadingFlags & static_cast<uint32_t>(FileLoadingFlags::PreMultiplyVertexColors);
		const bool flipY = fileLoadingFlags & static_cast<uint32_t>(FileLoadingFlags::FlipY);

		//std::cout << "materials.size(): " << materials.size() << std::endl;

		for (TFNode linearnode : linearNodes) {

			const glm::mat4 localMatrix = linearnode.GetMatrix();

			for (TFMesh::Primitive& primitive : linearnode.mesh.primitives) {

				for (uint32_t i = 0; i < primitive.vertexCount; i++) {

					Vertex& vertex = vertexBuffer[primitive.firstVertex + i];

					// Pre-transform vertex positions by node-hierarchy
					if (preTransform) {
						vertex.pos = glm::vec3(localMatrix * glm::vec4(vertex.pos, 1.0f));
						vertex.normal = glm::normalize(glm::mat3(localMatrix) * vertex.normal);
					}

					// Flip Y-Axis of vertex positions
					if (flipY) {
						//vertex.pos.x *= -1.0f;
						vertex.pos.y *= -1.0f;
						//vertex.pos.z *= -1.0f;
						vertex.normal.y *= -1.0f;
					}

					// Pre-Multiply vertex colors with material base color
					if (preMultiplyColor) {
						vertex.color = primitive.material.baseColorFactor * vertex.color;
					}
				}
			}
		}
	}

	//set metallic roughness workflow according to extension
	for (auto extension : gltfModel.extensionsUsed) {
		if (extension == "KHR_materials_pbrSpecularGlossiness") {
			//std::cout << "Required extension: " << extension;
			metallicRoughnessWorkflow = false;
		}
	}

	//init vertices and indices
	size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
	size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	indices.count = static_cast<uint32_t>(indexBuffer.size());
	vertices.count = static_cast<uint32_t>(vertexBuffer.size());

	//check if vertex and indices vectors contain data
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
		vertexBuffer.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel vertex staging buffer");
	}

	// Index data
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		&indexStaging,
		indexBufferSize,
		indexBuffer.data()) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel index staging buffer");
	}

	// Create device local buffers
	//vertices buffer names
	vertices.verticesBuffer.bufferData.bufferName = "gltfModelVerticesBuffer";
	vertices.verticesBuffer.bufferData.bufferMemoryName = "gltfModelVerticesBufferMemory";

	//created vertices buffer
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&vertices.verticesBuffer,
		vertexBufferSize,
		nullptr) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel vertices buffer");
	}

	//indices buffer and memory names
	indices.indicesBuffer.bufferData.bufferName = "gltfModelIndicesBuffer";
	indices.indicesBuffer.bufferData.bufferMemoryName = "gltfModelIndicesBufferMemory";

	// Index buffer
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&indices.indicesBuffer,
		indexBufferSize,
		nullptr) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltfmodel indices buffer");
	}

	//copy from staging buffers
	//create temporary command buffer
	VkCommandBuffer cmdBuffer = pCoreBase->objCreate.VKCreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);

	VkBufferCopy copyRegion{};

	copyRegion.size = vertexBufferSize;
	vkCmdCopyBuffer(cmdBuffer, vertexStaging.bufferData.buffer, vertices.verticesBuffer.bufferData.buffer, 1, &copyRegion);

	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(cmdBuffer, indexStaging.bufferData.buffer, indices.indicesBuffer.bufferData.buffer, 1, &copyRegion);

	//submit temporary command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//destroy staging buffers
	vkDestroyBuffer(pCoreBase->devices.logical, vertexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, vertexStaging.bufferData.memory, nullptr);
	vkDestroyBuffer(pCoreBase->devices.logical, indexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, indexStaging.bufferData.memory, nullptr);

	//get scene dimensions
	GetSceneDimensions();

	//setup descriptors
	uint32_t uboCount{ 0 };
	uint32_t imageCount{ 0 };

	//increment ubo count for nodes
	for (auto node : linearNodes) {
		if (node.hasMesh) {
			uboCount++;
		}
	}

	//increment image count for materials
	for (auto material : materials) {
		if (material.baseColorTexture.index > -1) {
			imageCount++;
		}
	}

	//output image and ubo count
	//std::cout << "uboCount: " << uboCount << std::endl;
	//std::cout << "imageCount: " << imageCount << std::endl;

	//declare pool sizes
	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uboCount },
	};

	//resize pool sizes if there are images
	if (imageCount > 0) {
		if (descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageBaseColor)) {
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount });
		}
		if (descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageNormalMap)) {
			poolSizes.push_back({ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageCount });
		}
	}

	//create descriptor pool
	VkDescriptorPoolCreateInfo descriptorPoolCI{};
	descriptorPoolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	descriptorPoolCI.pPoolSizes = poolSizes.data();
	descriptorPoolCI.maxSets = uboCount + imageCount;

	if (vkCreateDescriptorPool(pCoreBase->devices.logical, &descriptorPoolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
		throw std::invalid_argument("failed to create gltf model descriptor pool");
	}

	//descriptors for per-node uniform buffers
	{
		//create if it hasn't already been created before
		if (descriptorSetLayoutUbo == VK_NULL_HANDLE) {


			VkDescriptorSetLayoutBinding uniformBufferBinding{};
			uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			uniformBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			uniformBufferBinding.binding = 0;
			uniformBufferBinding.descriptorCount = 1;

			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {
				uniformBufferBinding,
			};

			VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
			descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayoutCI.pBindings = setLayoutBindings.data();

			if (vkCreateDescriptorSetLayout(pCoreBase->devices.logical, &descriptorLayoutCI, nullptr, &descriptorSetLayoutUbo) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create gltf model descriptor set layout(ubo)");
			}
		}

		for (auto node : nodes) {
			PrepareNodeDescriptor(&node, descriptorSetLayoutUbo);
		}
	}

	// Descriptors for per-material images
	{

		if (descriptorSetLayoutImage == VK_NULL_HANDLE) {

			std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};


			if (descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageBaseColor)) {
				VkDescriptorSetLayoutBinding imageBaseColorBinding{};
				imageBaseColorBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				imageBaseColorBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				imageBaseColorBinding.binding = static_cast<uint32_t>(setLayoutBindings.size());
				imageBaseColorBinding.descriptorCount = 1;
				setLayoutBindings.push_back(imageBaseColorBinding);
			}
			if (descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageNormalMap)) {
				VkDescriptorSetLayoutBinding normalMapBinding{};
				normalMapBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				normalMapBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
				normalMapBinding.binding = static_cast<uint32_t>(setLayoutBindings.size());
				normalMapBinding.descriptorCount = 1;
				setLayoutBindings.push_back(normalMapBinding);
			}
			VkDescriptorSetLayoutCreateInfo descriptorLayoutCI{};
			descriptorLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorLayoutCI.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
			descriptorLayoutCI.pBindings = setLayoutBindings.data();
			if (vkCreateDescriptorSetLayout(pCoreBase->devices.logical, &descriptorLayoutCI, nullptr, &descriptorSetLayoutImage) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create gltf model descriptr set layout(image)");
			}
		}

		for (auto& material : materials) {
			if (material.baseColorTexture.index > -1) {

				//std::cout << "\ngltf texture descriptor set test 1" << std::endl;

				material.CreateDescriptorSet(this->pCoreBase, descriptorPool, descriptorSetLayoutImage, descriptorBindingFlags);

				//std::cout << "\ngltf texture descriptor set test 2" << std::endl;

			}
		}
	}

	//std::cout << "\nTFModel\n''''''''''''''''\nload from file function end " << "\n" << std::endl;
}

void TFModel::LoadImages(tinygltf::Model& model, VkQueue transferQueue) {

	//std::cout << "gltf model materials count: " << model.materials.size() << std::endl;
	//std::cout << "gltf model textures count: " << model.textures.size() << std::endl;

	int idx = 0;

	textures.resize(0);

	for (tinygltf::Image& image : model.images) {
		vrt::Texture* texture = new vrt::Texture(pCoreBase);
		texture->fromglTfImage(image, path, this->pCoreBase, transferQueue);
		texture->index = static_cast<uint32_t>(textures.size());
		//std::cout << "texture[" << idx << "] path: " << path << std::endl;
		//std::cout << "texture[" << idx << "] index: " << texture->index << std::endl;
		this->textures.push_back(*texture);
		++idx;
	}

}

vrt::Texture TFModel::GetTexture(uint32_t index) {
	if (index < textures.size()) {
		return textures[index];
	}
	return nullptr;
}



void TFModel::LoadMaterials(tinygltf::Model& model) {

	int idx = 0;

	for (tinygltf::Material& mat : model.materials) {

		TFMesh::Material material{};

		//std::cout << "material index[" << idx << "] material name: " << model.materials[idx].name << std::endl;

		if (mat.values.find("baseColorTexture") != mat.values.end()) {
			material.baseColorTexture = GetTexture(model.textures[mat.values["baseColorTexture"].TextureIndex()].source);
			//std::cout << "loaded baseColorTexture material" << std::endl;
		}

		// Metallic roughness workflow
		if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
			material.metallicRoughnessTexture = GetTexture(model.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source);
			//std::cout << "loaded metallicRoughnessTexture material" << std::endl;
		}

		if (mat.values.find("roughnessFactor") != mat.values.end()) {
			material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
			//std::cout << "loaded roughnessFactor material" << std::endl;
		}

		if (mat.values.find("metallicFactor") != mat.values.end()) {
			material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
			//std::cout << "loaded metallicFactor material" << std::endl;
		}

		if (mat.values.find("baseColorFactor") != mat.values.end()) {
			material.baseColorFactor = glm::vec4(static_cast<float>(*mat.values["baseColorFactor"].ColorFactor().data()));
			//std::cout << "loaded baseColorFactor material" << std::endl;
		}

		if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
			material.normalTexture = GetTexture(model.textures[mat.additionalValues["normalTexture"].TextureIndex()].source);
			//std::cout << "loaded normalTexture material" << std::endl;
		}

		else {
			material.normalTexture = material.emptyTexture;
		}

		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
			material.emissiveTexture = GetTexture(model.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source);
			//std::cout << "loaded emissiveTexture material" << std::endl;
		}

		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
			material.occlusionTexture = GetTexture(model.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source);
			//std::cout << "loaded occlusionTexture material" << std::endl;
		}

		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {

			tinygltf::Parameter param = mat.additionalValues["alphaMode"];

			if (param.string_value == "BLEND") {
				material.alphaMode = TFMesh::Material::AlphaMode::ALPHAMODE_BLEND;
				//std::cout << "loaded blend material" << std::endl;
			}

			if (param.string_value == "MASK") {
				material.alphaMode = TFMesh::Material::AlphaMode::ALPHAMODE_MASK;
				//std::cout << "loaded mask material" << std::endl;
			}

		}

		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
			//std::cout << "loaded alphaCutoff material" << std::endl;
		}

		materials.push_back(material);
		idx++;
	}

	//*!@#*!*@#)!@_#!)%!(%$(!@#_!@#!(@#*!%!@_# this motherfucker down here...
	// 
	// Push a default material at the end of the list for meshes with no material assigned
	materials.push_back(TFMesh::Material{});

}

void TFModel::LoadNode(TFNode* parent, const tinygltf::Node& node, uint32_t nodeIndex,
	const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale) {

	//std::cout << "\n'''''''''\nload node\n'''''''''\nbegin function\n--------------" << std::endl;

	TFNode* newNode = new TFNode(this->pCoreBase);

	newNode->index = nodeIndex;
	newNode->parentNode = parent;
	newNode->name = node.name;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	//std::cout << "node name: " << node.name << std::endl;
	//std::cout << "node index: " << nodeIndex << std::endl;
	//if (parent == nullptr) {
	//	//std::cout << "node parent: nullptr" << std::endl;
	//}
	//
	//else {
	//	//std::cout << "node parent: " << parent->name << std::endl;
	//}


	if (node.mesh >= 0) {
		newNode->hasMesh = true;
	}

	// Generate local node matrix
	//std::cout << "node[" << node.name << "] translation size: " << node.translation.size() << std::endl;
	glm::vec3 translation = glm::vec3(0.0f);
	if (node.translation.size() == 3) {
		translation = glm::make_vec3(node.translation.data());
		newNode->translation = translation;
	}

	//std::cout << "node[" << node.name << "] rotation size: " << node.rotation.size() << std::endl;
	glm::mat4 rotation = glm::mat4(1.0f);
	if (node.rotation.size() == 4) {
		glm::quat q = glm::make_quat(node.rotation.data());
		newNode->rotation = glm::mat4(q);
	}


	//std::cout << "node[" << node.name << "] scale size: " << node.scale.size() << std::endl;
	glm::vec3 scale = glm::vec3(1.0f);
	if (node.scale.size() == 3) {
		scale = glm::make_vec3(node.scale.data());
		newNode->scale = scale;
	}

	//std::cout << "node[" << node.name << "] matrix size: " << node.matrix.size() << std::endl;
	if (node.matrix.size() == 16) {
		newNode->matrix = glm::make_mat4x4(node.matrix.data());
		//if (globalscale != 1.0f) {
		//}
	}

	// Create translation matrix
	glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);

	// Create scaling matrix
	glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	// Combine matrices (order: translation * rotation * scale)
	newNode->matrix = glm::mat4(1.0f) * translationMatrix * rotation * scaleMatrix;

	// Node with children
	if (node.children.size() > 0) {
		//std::cout << "\nnode has children -- loading children nodes\n-------------------------------------------" << std::endl;
		for (auto i = 0; i < node.children.size(); i++) {
			//std::cout << "\n----------------------\nnode children [" << i << "] :" << std::endl;
			LoadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
		}
	}

	// Node contains mesh data
	if (node.mesh > -1) {
		//std::cout << "\nnode contains mesh -- creating mesh\n-----------------------------------\n" << std::endl;

		//reference tinyglft::Mesh
		const tinygltf::Mesh mesh = model.meshes[node.mesh];

		//init instance of TFMesh
		TFMesh newMesh = TFMesh(pCoreBase, newNode->matrix, mesh.name);
		//std::cout << "\nMesh Name: " << newMesh.meshName << std::endl;

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
			// Vertices
			{
				const float* bufferPos = nullptr;
				const float* bufferNormals = nullptr;
				const float* bufferTexCoords = nullptr;
				const float* bufferColors = nullptr;
				const float* bufferTangents = nullptr;
				uint32_t numColorComponents = 0;
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

				if (primitive.attributes.find("COLOR_0") != primitive.attributes.end())
				{
					const tinygltf::Accessor& colorAccessor = model.accessors[primitive.attributes.find("COLOR_0")->second];
					const tinygltf::BufferView& colorView = model.bufferViews[colorAccessor.bufferView];
					// Color buffer are either of type vec3 or vec4
					numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
					bufferColors = reinterpret_cast<const float*>(&(model.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
				}

				if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
				{
					const tinygltf::Accessor& tangentAccessor = model.accessors[primitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& tangentView = model.bufferViews[tangentAccessor.bufferView];
					bufferTangents = reinterpret_cast<const float*>(&(model.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
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

				hasSkin = (bufferJoints && bufferWeights);

				vertexCount = static_cast<uint32_t>(posAccessor.count);

				for (size_t v = 0; v < posAccessor.count; v++) {
					Vertex vert{};
					vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * 3]), 1.0f);
					vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * 3]) : glm::vec3(0.0f)));
					vert.uv = bufferTexCoords ? glm::make_vec2(&bufferTexCoords[v * 2]) : glm::vec3(0.0f);
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

			//primitive instance
			TFMesh::Primitive newPrimitive{};
			newPrimitive.firstVertex = vertexStart;
			newPrimitive.vertexCount = vertexCount;
			newPrimitive.firstIndex = indexStart;
			newPrimitive.indexCount = indexCount;

			//set primitive dimensions
			newPrimitive.SetDimensions(posMin, posMax);

			//mesh primitive references
			newMesh.primitives.push_back(newPrimitive);
		}

		////add mesh to node
		newNode->mesh = newMesh;
	}

	if (parent != nullptr) {
		parent->children.push_back(newNode);
		//std::cout << "child node pushed back to parent" << std::endl;
	}

	else {
		nodes.push_back(*newNode);
		//std::cout << "node pushed back new node" << std::endl;
	}

	linearNodes.push_back(*newNode);
	//std::cout << "linear nodes pushed back new node" << std::endl;

	//std::cout << "\n---------------------\nend of load node func\n---------------------\n" << std::endl;

}

void TFModel::LoadSkins(tinygltf::Model& model) {
	//std::cout << " 111111111111111LOADING SKINS" << std::endl;
	for (tinygltf::Skin& source : model.skins) {
		//std::cout << "num skins: " << model.skins.size() << std::endl;
		TFNode::Skin newSkin;
		newSkin.name = source.name;

		// Find skeleton root node
		if (source.skeleton > -1) {
			newSkin.skeletonRoot = NodeFromIndex(source.skeleton);
		}

		// Find joint nodes
		for (int jointIndex : source.joints) {
			TFNode* node = NodeFromIndex(jointIndex);
			if (node) {
				newSkin.joints.push_back(NodeFromIndex(jointIndex));
			}
		}

		// Get inverse bind matrices from buffer
		if (source.inverseBindMatrices > -1) {
			const tinygltf::Accessor& accessor = model.accessors[source.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			newSkin.inverseBindMatrices.resize(accessor.count);
			memcpy(newSkin.inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));

			// Store inverse bind matrices for this skin in a shader storage buffer object
			// To keep this sample simple, we create a host visible shader storage buffer

			newSkin.ssbo.bufferData.bufferName = newSkin.name + "_skinSSBOBuffer";
			newSkin.ssbo.bufferData.bufferMemoryName = newSkin.name + "_skinSSBOBufferMemory";

			if (this->pCoreBase->CreateBuffer(
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&newSkin.ssbo,
				sizeof(glm::mat4) * newSkin.inverseBindMatrices.size(),
				newSkin.inverseBindMatrices.data()) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create skin ssbo");
			}
			//if(newSkin.ssbo.map() != VK_SUCCESS) {
			//	throw std::invalid_argument("failed to MAP skin ssbo");
			//}

		}

		skins.push_back(newSkin);
	}

}

TFNode* TFModel::FindNode(TFNode* parent, uint32_t index) {
	TFNode* nodeFound = nullptr;
	if (parent->index == index) {
		return parent;
	}
	for (auto& child : parent->children) {
		nodeFound = FindNode(child, index);
		if (nodeFound) {
			break;
		}
	}
	//std::cout << " returning nullptr node" << std::endl;
	return nodeFound;
}

TFNode* TFModel::NodeFromIndex(uint32_t index) {
	//std::cout << " NodeFromIndex" << std::endl;
	TFNode* nodeFound = nullptr;
	for (auto& node : nodes) {
		//std::cout << " FindNode -- nodeformindex" << std::endl;
		nodeFound = FindNode(&node, index);
		if (nodeFound) {
			break;
		}
	}
	return nodeFound;
}

void TFModel::LoadAnimations(tinygltf::Model model) {

	animations.resize(model.animations.size());

	for (size_t i = 0; i < model.animations.size(); i++)
	{
		//tinygltf::Animation glTFAnimation = model.animations[i];
		animations[i].name = model.animations[i].name;

		// Samplers
		animations[i].samplers.resize(model.animations[i].samplers.size());

		//std::cout << "animations[i].samplers.size(): " << animations[i].samplers.size() << std::endl;

		for (size_t j = 0; j < model.animations[i].samplers.size(); j++)
		{
			//tinygltf::AnimationSampler glTFSampler = model.animations[i].samplers[j];
			//AnimationSampler& animations[i].samplers[j] = animations[i].samplers[j];
			animations[i].samplers[j].interpolation = model.animations[i].samplers[j].interpolation;

			// Read sampler keyframe input time values
			{
				//const tinygltf::Accessor& accessor = model.accessors[model.animations[i].samplers[j].input];
				//const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				//const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				const void* dataPtr =
					&model.buffers[model.bufferViews[model.accessors[model.animations[i].samplers[j].input].bufferView].buffer].data[
						model.accessors[model.animations[i].samplers[j].input].byteOffset +
					model.bufferViews[model.accessors[model.animations[i].samplers[j].input].bufferView].byteOffset];
				//const float* buf = static_cast<const float*>(dataPtr);
				for (size_t index = 0; index < model.accessors[model.animations[i].samplers[j].input].count; index++)
				{
					animations[i].samplers[j].inputs.push_back(static_cast<const float*>(dataPtr)[index]);
				}
				// Adjust animation's start and end times
				for (auto input : animations[i].samplers[j].inputs)
				{
					if (input < animations[i].start)
					{
						animations[i].start = input;
					};
					if (input > animations[i].end)
					{
						animations[i].end = input;
					}
				}
			}

			// Read sampler keyframe output translate/rotate/scale values
			{
				//const tinygltf::Accessor& accessor = model.accessors[model.animations[i].samplers[j].output];
				//const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				//const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				const void* dataPtr =
					&model.buffers[model.bufferViews[model.accessors[model.animations[i].samplers[j].output].bufferView].buffer].data[
						model.accessors[model.animations[i].samplers[j].output].byteOffset +
							model.bufferViews[model.accessors[model.animations[i].samplers[j].output].bufferView].byteOffset];
				switch (model.accessors[model.animations[i].samplers[j].output].type)
				{
				case TINYGLTF_TYPE_VEC3: {
					//const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
					for (size_t index = 0; index < model.accessors[model.animations[i].samplers[j].output].count; index++)
					{
						animations[i].samplers[j].outputsVec4.push_back(glm::vec4(static_cast<const glm::vec3*>(dataPtr)[index], 0.0f));
					}
					break;
				}
				case TINYGLTF_TYPE_VEC4: {
					//const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
					for (size_t index = 0; index < model.accessors[model.animations[i].samplers[j].output].count; index++)
					{
						animations[i].samplers[j].outputsVec4.push_back(static_cast<const glm::vec4*>(dataPtr)[index]);
					}
					break;
				}
				default: {
					//std::cout << "unknown type" << std::endl;
					break;
				}
				}
			}
		}

		// Channels
		animations[i].channels.resize(model.animations[i].channels.size());
		for (size_t j = 0; j < model.animations[i].channels.size(); j++)
		{
			//tinygltf::AnimationChannel glTFChannel = model.animations[i].channels[j];
			//AnimationChannel& dstChannel = animations[i].channels[j];
			animations[i].channels[j].path = model.animations[i].channels[j].target_path;
			animations[i].channels[j].samplerIndex = model.animations[i].channels[j].sampler;
			animations[i].channels[j].node = NodeFromIndex(model.animations[i].channels[j].target_node);
		}
	}
}

void TFModel::GetNodeDimensions(TFNode* node, glm::vec3& min, glm::vec3& max) {
	if (node->hasMesh) {
		for (TFMesh::Primitive primitive : node->mesh.primitives) {
			glm::vec4 locMin = glm::vec4(primitive.dimensions.min, 1.0f) * node->GetMatrix();
			glm::vec4 locMax = glm::vec4(primitive.dimensions.max, 1.0f) * node->GetMatrix();
			if (locMin.x < min.x) { min.x = locMin.x; }
			if (locMin.y < min.y) { min.y = locMin.y; }
			if (locMin.z < min.z) { min.z = locMin.z; }
			if (locMax.x > max.x) { max.x = locMax.x; }
			if (locMax.y > max.y) { max.y = locMax.y; }
			if (locMax.z > max.z) { max.z = locMax.z; }
		}
	}
	for (auto child : node->children) {
		GetNodeDimensions(child, min, max);
	}
}

void TFModel::GetSceneDimensions() {
	dimensions.min = glm::vec3(FLT_MAX);
	dimensions.max = glm::vec3(-FLT_MAX);
	for (auto node : nodes) {
		GetNodeDimensions(&node, dimensions.min, dimensions.max);
	}
	dimensions.size = dimensions.max - dimensions.min;
	dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
	dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
}

void TFModel::PrepareNodeDescriptor(TFNode* node, VkDescriptorSetLayout descriptorSetLayout) {

	//std::cout << "\n attempting to prepare node descriptor\n" << std::endl;

	if (node->hasMesh) {

		//std::cout << "\n attempting to prepare node descriptor in progress1\n" << std::endl;
		//std::cout << "1prep node descriptor_node name: " << node->name << std::endl;
		//std::cout << "2prep node descriptor_buffer name: " << node->mesh.uniformBufferData.ubo.bufferData.bufferName << std::endl;

		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = descriptorPool;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;

		if (vkAllocateDescriptorSets(pCoreBase->devices.logical, &descriptorSetAllocInfo, &node->mesh.uniformBufferData.descriptorSet) != VK_SUCCESS) {
			throw std::invalid_argument("failed to allocate gltf model descriptor sets");
		}

		node->mesh.uniformBufferData.descriptor.offset = 0;
		node->mesh.uniformBufferData.descriptor.buffer = node->mesh.uniformBufferData.ubo.bufferData.buffer;
		node->mesh.uniformBufferData.descriptor.range = sizeof(node->mesh.uniformBlock);

		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.dstSet = node->mesh.uniformBufferData.descriptorSet;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.pBufferInfo = &node->mesh.uniformBufferData.descriptor;

		vkUpdateDescriptorSets(pCoreBase->devices.logical, 1, &writeDescriptorSet, 0, nullptr);

		for (auto& child : node->children) {
			PrepareNodeDescriptor(child, descriptorSetLayout);
		}
	}
}

void TFModel::UpdateAnimation(uint32_t index, float time) {
	modelTransformMatrix = glm::mat4(1.0f);
	////std::cout << " animations.channels.size(): " << animations[index].channels.size() << std::endl;

	if (index > static_cast<uint32_t>(animations.size()) - 1)
	{
		//std::cout << "No animation with index " << index << std::endl;
		return;
	}
	//Animation& animation = animations[index];
	animations[index].currentTime += time;
	if (animations[index].currentTime > animations[index].end)
	{
		animations[index].currentTime -= animations[index].end;
	}

	for (auto& channel : animations[index].channels)
	{
		AnimationSampler& sampler = animations[index].samplers[channel.samplerIndex];
		for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
		{
			if (sampler.interpolation != "LINEAR")
			{
				//std::cout << "This sample only supports linear interpolations\n";
				continue;
			}

			// Inside your loop where animation updates occur
			if ((animations[index].currentTime >= sampler.inputs[i]) && (animations[index].currentTime <= sampler.inputs[i + 1]))
			{
				float a = (animations[index].currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
				////std::cout << "Animation Progress (a): " << a << std::endl;

				if (channel.path == "translation")
				{
					////std::cout << "Updating translation for node: " << channel.node->name << std::endl;
					////std::cout << "Current Translation: " << channel.node->translation.x << ", " << channel.node->translation.y << ", " << channel.node->translation.z << std::endl;
					////std::cout << "Previous Translation: " << sampler.outputsVec4[i].x << ", " << sampler.outputsVec4[i].y << ", " << sampler.outputsVec4[i].z << std::endl;
					////std::cout << "Next Translation: " << sampler.outputsVec4[i + 1].x << ", " << sampler.outputsVec4[i + 1].y << ", " << sampler.outputsVec4[i + 1].z << std::endl;
					channel.node->translation = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
					//modelTransformMatrix = glm::translate(modelTransformMatrix, channel.node->translation);
				}
				if (channel.path == "rotation")
				{
					////std::cout << "Updating rotation for node: " << channel.node->name << std::endl;
					////std::cout << "Current Rotation: " << channel.node->rotation.x << ", " << channel.node->rotation.y << ", " << channel.node->rotation.z << ", " << channel.node->rotation.w << std::endl;
					////std::cout << "Previous Rotation: " << sampler.outputsVec4[i].x << ", " << sampler.outputsVec4[i].y << ", " << sampler.outputsVec4[i].z << ", " << sampler.outputsVec4[i].w << std::endl;
					////std::cout << "Next Rotation: " << sampler.outputsVec4[i + 1].x << ", " << sampler.outputsVec4[i + 1].y << ", " << sampler.outputsVec4[i + 1].z << ", " << sampler.outputsVec4[i + 1].w << std::endl;

					glm::quat q1(sampler.outputsVec4[i].w, sampler.outputsVec4[i].x, sampler.outputsVec4[i].y, sampler.outputsVec4[i].z);
					glm::quat q2(sampler.outputsVec4[i + 1].w, sampler.outputsVec4[i + 1].x, sampler.outputsVec4[i + 1].y, sampler.outputsVec4[i + 1].z);

					channel.node->rotation = glm::normalize(glm::slerp(q1, q2, a));
					//modelTransformMatrix *= glm::mat4_cast(channel.node->rotation);
				}
				if (channel.path == "scale")
				{
					////std::cout << "Updating scale for node: " << channel.node->name << std::endl;
					////std::cout << "Current Scale: " << channel.node->scale.x << ", " << channel.node->scale.y << ", " << channel.node->scale.z << std::endl;
					////std::cout << "Previous Scale: " << sampler.outputsVec4[i].x << ", " << sampler.outputsVec4[i].y << ", " << sampler.outputsVec4[i].z << std::endl;
					////std::cout << "Next Scale: " << sampler.outputsVec4[i + 1].x << ", " << sampler.outputsVec4[i + 1].y << ", " << sampler.outputsVec4[i + 1].z << std::endl;
					channel.node->scale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
					//modelTransformMatrix = glm::scale(modelTransformMatrix, channel.node->scale);
				}
			}

		}
	}

	//for (auto& node : nodes)
	//{
	//	// Apply the transformation to the node's model matrix
	//	// If you want to extract translation, rotation, and scale separately:
	//	// glm::vec3 translation, rotation, scale;
	//	// glm::decompose(node.modelMatrix, scale, rotation, translation, glm::vec3(), glm::vec3(), glm::quat());
	//	// node.translation = translation;
	//	// node.rotation = rotation;
	//	// node.scale = scale;
	//	// Note: glm::decompose is available in newer versions of GLM (0.9.9 and later)
	//}

	for (auto& node : nodes)
	{
		UpdateJoints(&node);
		//node.matrix = modelTransformMatrix * node.GetMatrix();
	}
}

//void TFModel::UpdateAnimation(uint32_t index, float time) {
//	if (index > static_cast<uint32_t>(animations.size()) - 1) {
//		//std::cout << "No animation with index " << index << std::endl;
//		return;
//	}
//
//	// Update animation time
//	animations[index].currentTime += time;
//	if (animations[index].currentTime > animations[index].end) {
//		animations[index].currentTime -= animations[index].end;
//	}
//
//	// Loop through nodes and apply animation transformations directly to vertices
//	for (auto& linearnode : linearNodes) {
//		const glm::mat4 localMatrix = linearnode.GetMatrix();
//
//		for (TFMesh::Primitive& primitive : linearnode.mesh.primitives) {
//			for (uint32_t i = 0; i < primitive.vertexCount; i++) {
//				//Vertex& vertex = vertexBuffer[primitive.firstVertex + i];
//
//				// Apply animation transformations directly to vertices
//				for (auto& channel : animations[index].channels) {
//					AnimationSampler& sampler = animations[index].samplers[channel.samplerIndex];
//					for (size_t j = 0; j < sampler.inputs.size() - 1; j++) {
//						if ((animations[index].currentTime >= sampler.inputs[j]) && (animations[index].currentTime <= sampler.inputs[j + 1])) {
//							float a = (animations[index].currentTime - sampler.inputs[j]) / (sampler.inputs[j + 1] - sampler.inputs[j]);
//
//							if (channel.path == "translation") {
//								glm::vec3 translation = glm::mix(sampler.outputsVec4[j], sampler.outputsVec4[j + 1], a);
//								vertexBuffer[primitive.firstVertex + i].pos += translation;
//							}
//							else if (channel.path == "rotation") {
//								glm::quat q1(sampler.outputsVec4[j].w, sampler.outputsVec4[j].x, sampler.outputsVec4[j].y, sampler.outputsVec4[j].z);
//								glm::quat q2(sampler.outputsVec4[j + 1].w, sampler.outputsVec4[j + 1].x, sampler.outputsVec4[j + 1].y, sampler.outputsVec4[j + 1].z);
//								glm::quat rotation = glm::normalize(glm::slerp(q1, q2, a));
//								vertexBuffer[primitive.firstVertex + i].pos = rotation * vertexBuffer[primitive.firstVertex + i].pos;
//								vertexBuffer[primitive.firstVertex + i].normal = rotation * vertexBuffer[primitive.firstVertex + i].normal;
//							}
//							else if (channel.path == "scale") {
//								glm::vec3 scale = glm::mix(sampler.outputsVec4[j], sampler.outputsVec4[j + 1], a);
//								vertexBuffer[primitive.firstVertex + i].pos *= scale;
//								vertexBuffer[primitive.firstVertex + i].normal *= scale;
//							}
//						}
//					}
//				}
//			}
//		}
//	}
//	for (auto& node : nodes) {
//				UpdateJoints(&node);
//			}
//}


void TFModel::UpdateJoints(TFNode* node) {
	if (node->skin != nullptr)
	{
		// Update the joint matrices
		glm::mat4              inverseTransform = glm::inverse(GetNodeMatrix(node));
		//TFNode::Skin           skin = skins[0];
		size_t                 numJoints = (uint32_t)skins[0].joints.size();
		std::vector<glm::mat4> jointMatrices(numJoints);
		for (size_t i = 0; i < numJoints; i++)
		{
			jointMatrices[i] = GetNodeMatrix(skins[0].joints[i]) * skins[0].inverseBindMatrices[i];
			jointMatrices[i] = inverseTransform * jointMatrices[i];
		}
		// Update ssbo
		skins[0].ssbo.copyTo(jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
	}

	for (auto& child : node->children)
	{
		UpdateJoints(child);
	}
}

void TFModel::UpdateVertexIndex(glm::mat4 transform) {
	//for (TFNode linearnode : linearNodes) {
	//
	//	const glm::mat4 localMatrix = linearnode.GetMatrix();
	//
	//	for (TFMesh::Primitive& primitive : linearnode.mesh.primitives) {
	//
	//		for (uint32_t i = 0; i < primitive.vertexCount; i++) {
	//
	//			Vertex& vertex = vertexBuffer[primitive.firstVertex + i];
	//
	//			// Pre-transform vertex positions by node-hierarchy
	//			if (preTransform) {
	//				vertex.pos = glm::vec3(localMatrix * glm::vec4(vertex.pos, 1.0f));
	//				vertex.normal = glm::normalize(glm::mat3(localMatrix) * vertex.normal);
	//			}
	//
	//			// Flip Y-Axis of vertex positions
	//			if (flipY) {
	//				vertex.pos.y *= -1.0f;
	//				vertex.normal.y *= -1.0f;
	//			}
	//
	//			// Pre-Multiply vertex colors with material base color
	//			if (preMultiplyColor) {
	//				vertex.color = primitive.material.baseColorFactor * vertex.color;
	//			}
	//		}
	//	}
	//}
}

void TFModel::DestroyModel() {

	//ssbo
	for (auto skin : this->skins) {
		skin.ssbo.destroy(this->pCoreBase->devices.logical);
	}

	//descriptors
	vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, descriptorSetLayoutImage, nullptr);
	vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, descriptorSetLayoutUbo, nullptr);
	vkDestroyDescriptorPool(this->pCoreBase->devices.logical, this->descriptorPool, nullptr);

	//vertex and index buffer
	indices.indicesBuffer.destroy(this->pCoreBase->devices.logical);
	vertices.verticesBuffer.destroy(this->pCoreBase->devices.logical);

	//meshes
	for (int i = 0; i < this->linearNodes.size(); i++) {
		if (linearNodes[i].hasMesh) {
			linearNodes[i].mesh.uniformBufferData.ubo.destroy(this->pCoreBase->devices.logical);
		}
	}

	//textures
	int texIDX = 0;
	for (auto tex : textures) {
		tex.DestroyTexture();
		++texIDX;
	}
}

//void TFModel::Material::CreateDescriptorSet(CoreBase* coreBase, VkDescriptorPool descriptorPool,
//	VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags) {
//
//	//descriptor set allocate info
//	VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
//	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
//	descriptorSetAllocInfo.descriptorPool = descriptorPool;
//	descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
//	descriptorSetAllocInfo.descriptorSetCount = 1;
//
//	//allocate
//	if (vkAllocateDescriptorSets(coreBase->devices.logical, &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS) {
//		throw std::invalid_argument("failed to allocate gltf model MATERIAL descriptor set");
//	}
//
//	//image descriptor vector
//	std::vector<VkDescriptorImageInfo> imageDescriptors{};
//
//	//write set vector
//	std::vector<VkWriteDescriptorSet> writeDescriptorSets{};
//
//
//	if (descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageBaseColor)) {
//
//		imageDescriptors.push_back(baseColorTexture.descriptor);
//
//		baseColorTexture.descriptor.imageLayout = baseColorTexture.imageLayout;
//		baseColorTexture.descriptor.imageView = baseColorTexture.view;
//		baseColorTexture.descriptor.sampler = baseColorTexture.sampler;
//
//		VkWriteDescriptorSet writeDescriptorSet{};
//		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//		writeDescriptorSet.descriptorCount = 1;
//		writeDescriptorSet.dstSet = descriptorSet;
//		writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
//		writeDescriptorSet.pImageInfo = &baseColorTexture.descriptor;
//		writeDescriptorSets.push_back(writeDescriptorSet);
//	}
//
//	////std::cout << "\n INSIDE gltf texture descriptor set test 3" << std::endl;
//
//	if (normalTexture.index > -1 && descriptorBindingFlags & static_cast<uint32_t>(DescriptorBindingFlags::ImageNormalMap)) {
//		imageDescriptors.push_back(normalTexture.descriptor);
//
//		normalTexture.descriptor.imageLayout = normalTexture.imageLayout;
//		normalTexture.descriptor.imageView = normalTexture.view;
//		normalTexture.descriptor.sampler = normalTexture.sampler;
//
//		VkWriteDescriptorSet writeDescriptorSet{};
//		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
//		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//		writeDescriptorSet.descriptorCount = 1;
//		writeDescriptorSet.dstSet = descriptorSet;
//		writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
//		writeDescriptorSet.pImageInfo = &normalTexture.descriptor;
//		writeDescriptorSets.push_back(writeDescriptorSet);
//	}
//
//	////std::cout << "\n INSIDE gltf texture descriptor set test 4" << std::endl;
//
//	vkUpdateDescriptorSets(coreBase->devices.logical, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
//
//	////std::cout << "\n INSIDE gltf texture descriptor set test 5" << std::endl;
//
//}
