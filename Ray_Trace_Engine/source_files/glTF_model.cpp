#include "glTF_model.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

//#include <tiny_gltf.h>

//We use a custom image loading function with tinyglTF, so we can do custom stuff loading ktx textures
bool loadImageDataFunction(tinygltf::Image* image,
	const int imageIndex,
	std::string* error,
	std::string* warning,
	int req_width,
	int req_height,
	const unsigned char* bytes,
	int size,
	void* userData) {

	// KTX files will be handled by our own code
	if (image->uri.find_last_of(".") != std::string::npos) {
		if (image->uri.substr(image->uri.find_last_of(".") + 1) == "ktx") {
			return true;
		}
	}

	return tinygltf::LoadImageData(image, imageIndex, error, warning, req_width, req_height, bytes, size, userData);

}

bool loadImageDataFunctionEmpty(tinygltf::Image* image,
	const int imageIndex, std::string* error,
	std::string* warning,
	int req_width,
	int req_height,
	const unsigned char* bytes,
	int size,
	void* userData) {

	// This function will be used for samples that don't require images to be loaded
	return true;

}

glTF_model::glTF_model() {

}

glTF_model::glTF_model(CoreBase* coreBase, std::string fileName) {
	Init_glTF_model(coreBase, fileName);
}

void glTF_model::Init_glTF_model(CoreBase* coreBase, std::string fileName) {

	//locally set loading flags
	uint32_t local_gltfLoadingFlags =
		static_cast<uint64_t>(FileLoadingFlags::PreTransformVertices) |
		static_cast<uint64_t>(FileLoadingFlags::PreMultiplyVertexColors) |
		static_cast<uint64_t>(FileLoadingFlags::FlipY);

	//initialize core pointer for access to core components (including devices)
	this->pCoreBase = coreBase;

	//initialize device pointer for readability(can also be found in pCoreBase->devices.logical
	this->pDevice = &coreBase->devices.logical;

	//load gltf file
	this->LoadglTFFile(fileName, pCoreBase->queue.graphics, local_gltfLoadingFlags, 1.0f);

}

void glTF_model::LoadImages(tinygltf::Model& model, VkQueue transferQueue) {

	//std::cout << "gltf model materials count: " << model.materials.size() << std::endl;
	//std::cout << "gltf model textures count: " << model.textures.size() << std::endl;

	int idx = 0;

	this->images.resize(model.images.size());

	for (tinygltf::Image& image : model.images) {
		//vrt::Texture texture = vrt::Texture(pCoreBase);
		this->images[idx].texture = vrt::Texture(this->pCoreBase);
		this->images[idx].texture.fromglTfImage(image, path, this->pCoreBase, transferQueue);
		this->images[idx].texture.index = static_cast<uint32_t>(textures.size());
		//std::cout << "texture[" << idx << "] path: " << path << std::endl;
		//std::cout << "texture[" << idx << "] index: " << this->images[idx].texture.index << std::endl;
		//this->images.push_back(*texture);
		++idx;
	}

}

void glTF_model::LoadMaterials(tinygltf::Model& input) {
	materials.resize(input.materials.size());
	for (size_t i = 0; i < input.materials.size(); i++)
	{
		// We only read the most basic properties required for our sample
		tinygltf::Material glTFMaterial = input.materials[i];
		// Get the base color factor
		if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end())
		{
			materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
		}
		// Get base color texture index
		if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end())
		{
			materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
		}
	}
}

void glTF_model::LoadTextures(tinygltf::Model& input) {
	textures.resize(input.textures.size());
	for (size_t i = 0; i < input.textures.size(); i++)
	{
		textures[i].imageIndex = input.textures[i].source;
	}
}

void glTF_model::LoadNode(const tinygltf::Node& inputNode,
	const tinygltf::Model& input,
	glTF_model::Node* parent,
	uint32_t nodeIndex,
	std::vector<uint32_t>& indexBuffer,
	std::vector<glTF_model::Vertex>& vertexBuffer) {

	glTF_model::Node* node = new glTF_model::Node{};
	node->parent = parent;
	node->matrix = glm::mat4(1.0f);
	node->index = nodeIndex;
	node->skin = inputNode.skin;

	// Get the local node matrix
	// It's either made up from translation, rotation, scale or a 4x4 matrix
	if (inputNode.translation.size() == 3) {
		node->translation = glm::vec3(1.0f);
		node->translation = glm::make_vec3(inputNode.translation.data());
		//std::cout << "Translation: [" << node->translation.x << ", " << node->translation.y << ", " << node->translation.z << "]" << std::endl;
	}

	if (inputNode.rotation.size() == 4) {
		node->rotation = glm::mat4(1.0f);
		glm::quat q = glm::make_quat(inputNode.rotation.data());

		// Create a quaternion representing the desired rotation around the y-axis
		//glm::quat yRotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

		// Combine the original quaternion with the y-axis rotation
		//q = yRotation * q;

		// Convert the combined quaternion to a rotation matrix
		node->rotation = glm::mat4(q);

		//std::cout << "Rotation: [" << q.x << ", " << q.y << ", " << q.z << ", " << q.w << "]" << std::endl;
	}

	if (inputNode.scale.size() == 3) {
		node->scale = glm::vec3(1.0f);
		node->scale = glm::make_vec3(inputNode.scale.data());
		//std::cout << "Scale: [" << node->scale.x << ", " << node->scale.y << ", " << node->scale.z << "]" << std::endl;
	}

	if (inputNode.matrix.size() == 16) {
		node->matrix = glm::mat4(1.0f);
		node->matrix = glm::make_mat4x4(inputNode.matrix.data());
		//std::cout << "Matrix:" << std::endl;
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				//std::cout << node->matrix[row][col] << "\t";
			}
			//std::cout << std::endl;
		}
	};

	// Load node's children
	if (inputNode.children.size() > 0)
	{
		//std::cout << "Loading children..." << std::endl;
		for (size_t i = 0; i < inputNode.children.size(); i++)
		{
			LoadNode(input.nodes[inputNode.children[i]], input, node, inputNode.children[i], indexBuffer, vertexBuffer);
		}
	}

	// If the node contains mesh data, we load vertices and indices from the buffers
	// In glTF this is done via accessors and buffer views
	if (inputNode.mesh > -1) {
		//std::cout << "Loading mesh data for " << inputNode.name << std::endl;
		const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
		// Iterate through all primitives of this node's mesh
		for (size_t i = 0; i < mesh.primitives.size(); i++) {
			const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
			uint32_t                   firstIndex = static_cast<uint32_t>(indexBuffer.size());
			uint32_t                   vertexStart = static_cast<uint32_t>(vertexBuffer.size());
			uint32_t                   indexCount = 0;
			bool                       hasSkin = false;
			// Vertices
			{
				const float* positionBuffer = nullptr;
				const float* normalsBuffer = nullptr;
				const float* texCoordsBuffer = nullptr;
				const float* colorsBuffer = nullptr;
				const float* tangentsBuffer = nullptr;
				uint32_t numColorComponents = 0;
				const uint16_t* jointsBuffer = nullptr;
				const float* weightsBuffer = nullptr;
				size_t          vertexCount = 0;

				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& positionAccessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
					const tinygltf::BufferView& view = input.bufferViews[positionAccessor.bufferView];
					positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[positionAccessor.byteOffset + view.byteOffset]));
					vertexCount = positionAccessor.count;
				}

				// Get buffer data for vertex normals
				if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& normalAccessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& view = input.bufferViews[normalAccessor.bufferView];
					normalsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[normalAccessor.byteOffset + view.byteOffset]));
				}
				// Get buffer data for vertex texture coordinates
				// glTF supports multiple sets, we only load the first one
				if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& texAccessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[texAccessor.bufferView];
					texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[texAccessor.byteOffset + view.byteOffset]));
				}

				if (glTFPrimitive.attributes.find("COLOR_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& colorAccessor = input.accessors[glTFPrimitive.attributes.find("COLOR_0")->second];
					const tinygltf::BufferView& colorView = input.bufferViews[colorAccessor.bufferView];
					// Color buffer are either of type vec3 or vec4
					numColorComponents = colorAccessor.type == TINYGLTF_PARAMETER_TYPE_FLOAT_VEC3 ? 3 : 4;
					colorsBuffer = reinterpret_cast<const float*>(&(input.buffers[colorView.buffer].data[colorAccessor.byteOffset + colorView.byteOffset]));
				}

				if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& tangentAccessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
					const tinygltf::BufferView& tangentView = input.bufferViews[tangentAccessor.bufferView];
					tangentsBuffer = reinterpret_cast<const float*>(&(input.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
				}

				// POI: Get buffer data required for vertex skinning
				// Get vertex joint indices
				if (glTFPrimitive.attributes.find("JOINTS_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& jointAccessor = input.accessors[glTFPrimitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[jointAccessor.bufferView];
					jointsBuffer = reinterpret_cast<const uint16_t*>(&(input.buffers[view.buffer].data[jointAccessor.byteOffset + view.byteOffset]));
				}
				// Get vertex joint weights
				if (glTFPrimitive.attributes.find("WEIGHTS_0") != glTFPrimitive.attributes.end())
				{
					const tinygltf::Accessor& weightAccessor = input.accessors[glTFPrimitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& view = input.bufferViews[weightAccessor.bufferView];
					weightsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[weightAccessor.byteOffset + view.byteOffset]));
				}

				hasSkin = (jointsBuffer && weightsBuffer);

				vertexBuffer.resize(vertexCount);
				// Append data to model's vertex buffer
				for (int i = 0; i < vertexBuffer.size(); i++) {
					//Vertex vert{};
					vertexBuffer[i].pos = glm::vec3(glm::make_vec3(&positionBuffer[i * 3]));
					vertexBuffer[i].normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[i * 3]) : glm::vec3(0.0f)));
					vertexBuffer[i].uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[i * 2]) : glm::vec2(0.0f);
					vertexBuffer[i].color = glm::vec4(1.0f);
					vertexBuffer[i].joint0 = hasSkin ? glm::vec4(glm::make_vec4(&jointsBuffer[i * 4])) : glm::vec4(0.0f);
					vertexBuffer[i].weight0 = hasSkin ? glm::make_vec4(&weightsBuffer[i * 4]) : glm::vec4(0.0f);
					vertexBuffer[i].tangent = tangentsBuffer ? glm::vec4(glm::make_vec4(&tangentsBuffer[i * 4])) : glm::vec4(0.0f);

					vertexBuffer[i].pos = glm::vec4(node->GetLocalMatrix() * glm::vec4(vertexBuffer[i].pos, 1.0f));
					vertexBuffer[i].normal = glm::normalize(node->GetLocalMatrix() * glm::vec4(vertexBuffer[i].normal, 1.0f));
					vertexBuffer[i].pos.y *= -1.0f;
					//vertexBuffer[i].pos.z *= -1.0f;
					//vertexBuffer[i].normal.y *= -1.0f;
				}

				// Add print statements here to output vertex-related information
				//std::cout << "Loaded " << vertexCount << " vertices." << std::endl;
			}
			// Indices
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

				indexCount += static_cast<uint32_t>(accessor.count);

				// glTF supports different component types of indices
				switch (accessor.componentType)
				{
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + bufferView.byteOffset]);
					for (size_t index = 0; index < accessor.count; index++)
					{
						indexBuffer.push_back(buf[index] + vertexStart);
					}
					break;
				}
				default:
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
					return;
				}

				// Add print statements here to output index-related information
				//std::cout << "Loaded " << indexCount << " indices." << std::endl;
			}
			// Additional print statements for primitive-related information
			//std::cout << "Primitive " << i << ": firstIndex = " << firstIndex << ", indexCount = " << indexCount << ", materialIndex = " << glTFPrimitive.material << std::endl;

			Primitive primitive{};
			primitive.firstIndex = firstIndex;
			primitive.indexCount = indexCount;
			primitive.materialIndex = glTFPrimitive.material;
			node->mesh.primitives.push_back(primitive);
		}
	}

	if (parent) {
		parent->children.push_back(node);
	}

	else {
		nodes.push_back(node);
	}

	if (node != nullptr) {
		linearNodes.push_back(node);
	}


}


void glTF_model::LoadSkins(tinygltf::Model& input) {
	skins.resize(input.skins.size());
	//std::cout << "skins.size(): " << skins.size() << std::endl;
	for (size_t i = 0; i < input.skins.size(); i++)
	{
		tinygltf::Skin glTFSkin = input.skins[i];

		skins[i].name = glTFSkin.name;
		// Find the root node of the skeleton
		skins[i].skeletonRoot = NodeFromIndex(glTFSkin.skeleton);

		// Find joint nodes
		for (int jointIndex : glTFSkin.joints)
		{
			Node* node = NodeFromIndex(jointIndex);
			if (node)
			{
				skins[i].joints.push_back(node);
			}
		}

		// Get the inverse bind matrices from the buffer associated to this skin
		if (glTFSkin.inverseBindMatrices > -1)
		{
			const tinygltf::Accessor& accessor = input.accessors[glTFSkin.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
			skins[i].inverseBindMatrices.resize(accessor.count);
			memcpy(skins[i].inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));

			// Store inverse bind matrices for this skin in a shader storage buffer object
			// To keep this sample simple, we create a host visible shader storage buffer
			if (this->pCoreBase->CreateBuffer(
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&skins[i].ssbo,
				sizeof(glm::mat4) * skins[i].inverseBindMatrices.size(),
				skins[i].inverseBindMatrices.data()) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create skins[" + std::to_string(i) + "].ssbo");
			}
			//already mapped in above func
			//if (skins[i].ssbo.map(this->pCoreBase->devices.logical) != VK_SUCCESS) {								
			//	throw std::invalid_argument("failed to map skins[" + std::to_string(i) + "].ssbo");
			//}
		}

		else {
			if (this->pCoreBase->CreateBuffer(
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				&skins[i].ssbo,
				sizeof(glm::mat4) * skins[i].inverseBindMatrices.size(),
				skins[i].inverseBindMatrices.data()) != VK_SUCCESS) {
				throw std::invalid_argument("failed to create skins[" + std::to_string(i) + "].ssbo");
			}
		}
	}
}

glTF_model::Node* glTF_model::FindNode(glTF_model::Node* parent, uint32_t index) {
	glTF_model::Node* nodeFound = nullptr;
	if (parent->index == index)
	{
		return parent;
	}
	for (auto& child : parent->children)
	{
		nodeFound = this->FindNode(child, index);
		if (nodeFound)
		{
			break;
		}
	}
	return nodeFound;
}

glTF_model::Node* glTF_model::NodeFromIndex(uint32_t index) {
	Node* nodeFound = nullptr;
	for (auto& node : nodes)
	{
		nodeFound = this->FindNode(node, index);
		if (nodeFound)
		{
			break;
		}
	}
	return nodeFound;
}

void glTF_model::LoadAnimations(tinygltf::Model& input) {
	animations.resize(input.animations.size());

	for (size_t i = 0; i < input.animations.size(); i++) {

		tinygltf::Animation glTFAnimation = input.animations[i];
		animations[i].name = glTFAnimation.name;

		// Samplers
		animations[i].samplers.resize(glTFAnimation.samplers.size());

		for (size_t j = 0; j < glTFAnimation.samplers.size(); j++) {

			tinygltf::AnimationSampler glTFSampler = glTFAnimation.samplers[j];
			AnimationSampler& dstSampler = animations[i].samplers[j];
			dstSampler.interpolation = glTFSampler.interpolation;

			// Read sampler keyframe input time values
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFSampler.input];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				const float* buf = static_cast<const float*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++)
				{
					dstSampler.inputs.push_back(buf[index]);
				}
				// Adjust animation's start and end times
				for (auto input : animations[i].samplers[j].inputs)
				{
					if (input < animations[i].start) {
						animations[i].start = input;
					};

					if (input > animations[i].end) {
						animations[i].end = input;
					}
				}
			}

			// Read sampler keyframe output translate/rotate/scale values
			{
				const tinygltf::Accessor& accessor = input.accessors[glTFSampler.output];
				const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];
				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				switch (accessor.type)
				{
				case TINYGLTF_TYPE_VEC3: {
					const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++)
					{
						dstSampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
					}
					break;
				}
				case TINYGLTF_TYPE_VEC4: {
					const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++)
					{
						dstSampler.outputsVec4.push_back(buf[index]);
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
		animations[i].channels.resize(glTFAnimation.channels.size());
		for (size_t j = 0; j < glTFAnimation.channels.size(); j++)
		{
			tinygltf::AnimationChannel glTFChannel = glTFAnimation.channels[j];
			AnimationChannel& dstChannel = animations[i].channels[j];
			dstChannel.path = glTFChannel.target_path;
			dstChannel.samplerIndex = glTFChannel.sampler;
			dstChannel.node = NodeFromIndex(glTFChannel.target_node);
		}
	}
}

void glTF_model::UpdateJoints(glTF_model::Node* node) {
	if (node->skin > -1)
	{
		// Update the joint matrices
		glm::mat4              inverseTransform = glm::inverse(GetNodeMatrix(node));
		Skin                   skin = skins[node->skin];
		size_t                 numJoints = (uint32_t)skin.joints.size();
		std::vector<glm::mat4> jointMatrices(numJoints);
		for (size_t i = 0; i < numJoints; i++)
		{
			jointMatrices[i] = GetNodeMatrix(skin.joints[i]) * skin.inverseBindMatrices[i];
			jointMatrices[i] = inverseTransform * jointMatrices[i];
		}
		// Update ssbo
		skin.ssbo.copyTo(jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
	}

	for (auto& child : node->children)
	{
		UpdateJoints(child);
	}
}

void glTF_model::Destroy_glTF_model() {

	//ssbo
	for (auto skin : this->skins) {
		skin.ssbo.destroy(this->pCoreBase->devices.logical);
	}

	//vertex and index buffer
	indices.buffer.destroy(this->pCoreBase->devices.logical);
	vertices.buffer.destroy(this->pCoreBase->devices.logical);

	// -- textures
	for (auto& img : images) {
		img.texture.DestroyTexture();
	}

}

glm::mat4 glTF_model::GetNodeMatrix(glTF_model::Node* node) {
	glm::mat4              nodeMatrix = node->GetLocalMatrix();
	glTF_model::Node* currentParent = node->parent;
	while (currentParent)
	{
		nodeMatrix = currentParent->GetLocalMatrix() * nodeMatrix;
		currentParent = currentParent->parent;
	}
	return nodeMatrix;
}

void glTF_model::LoadglTFFile(std::string filename,
	VkQueue transferQueue,
	uint32_t fileLoadingFlags,
	float scale) {

	//output begin function for console readability
	//std::cout << "\n------------------------------------------------------------------\n" << std::endl;
	//std::cout << "//\t\t\tStart of LoadglTFFile\t\t\t//" << std::endl;
	//std::cout << "\n------------------------------------------------------------------\n" << std::endl;

	//std::cout << "filename: " << filename << "\n" << std::endl;

	//tinygltf model and context
	tinygltf::Model gltfInput;
	tinygltf::TinyGLTF gltfContext;

	//strings for file loading func warnings
	std::string error;
	std::string warning;

	//set glTF_model member string: path
	size_t pos = filename.find_last_of('/');
	path = filename.substr(0, pos);

	//set image loader per file loading flags
	if (fileLoadingFlags & static_cast<uint64_t>(FileLoadingFlags::DontLoadImages)) {
		gltfContext.SetImageLoader(loadImageDataFunctionEmpty, nullptr);
	}

	else {
		gltfContext.SetImageLoader(loadImageDataFunction, nullptr);
	}


	//load model file
	bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfInput, &error, &warning, filename);

	if (fileLoaded) {
		//std::cout << "gltfContext.LoadASCIIFromFile success." << std::endl;
	}

	else {
		//std::cout << "gltfContext.LoadASCIIFromFile failed." << std::endl;
	}

	if (fileLoaded) {

		//std::cout << "\nloading images.\n" << std::endl;
		LoadImages(gltfInput, pCoreBase->queue.graphics);

		//std::cout << "\nloading materials.\n" << std::endl;
		LoadMaterials(gltfInput);

		//std::cout << "\nloading textures.\n" << std::endl;
		LoadTextures(gltfInput);

		//std::cout << "\nloading nodes.\n" << std::endl;
		const tinygltf::Scene& scene = gltfInput.scenes[gltfInput.defaultScene > -1 ? gltfInput.defaultScene : 0];
		for (size_t i = 0; i < scene.nodes.size(); i++) {
			const tinygltf::Node node = gltfInput.nodes[scene.nodes[i]];
			LoadNode(node, gltfInput, nullptr, scene.nodes[i], indexBuffer, vertexBuffer);
		}

		//std::cout << "\nloading skins.\n" << std::endl;
		LoadSkins(gltfInput);

		//std::cout << "\nloading animations.\n" << std::endl;
		LoadAnimations(gltfInput);

		// Calculate initial pose
		//std::cout << "\nupdating joints.\ncalculating initial pose\n" << std::endl;
		for (auto node : nodes) {
			UpdateJoints(node);
		}
	}

	else {
		//std::cout << "failed to load: " << filename << std::endl;
	}


	//for (Node* node : linearNodes) {
	//	if (node->mesh.primitives.size() > -1) {
	//		const glm::mat4 localMatrix = node->GetLocalMatrix();
	//				const glm::mat4 localMatrix = node->GetLocalMatrix();
	//				//vertex.pos = glm::vec4(localMatrix * glm::vec4(vertex.pos, 1.0f));
	//				//vertex.normal = glm::normalize(localMatrix * glm::vec4(vertex.normal, 1.0f));
	//				//	vertex.pos.y *= -1.0f;
	//				//	vertex.normal.y *= -1.0f;
	//				// Pre-transform vertex positions by node-hierarchy
	//				//if (preTransform) {
	//				//	vertex.pos = glm::vec3(localMatrix * glm::vec4(vertex.pos, 1.0f));
	//				//	vertex.normal = glm::normalize(glm::mat3(localMatrix) * vertex.normal);
	//				//}
	//				// Flip Y-Axis of vertex positions
	//				//if (flipY) {
	//				//}
	//				// Pre-Multiply vertex colors with material base color
	//				//if (preMultiplyColor) {
	//				//	vertex.color = primitive->material.baseColorFactor * vertex.color;
	//				//}
	//			}
	//		}

	// Create and upload vertex and index buffer
	size_t vertexBufferSize = vertexBuffer.size() * sizeof(glTF_model::Vertex);
	size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
	this->indices.count = static_cast<uint32_t>(indexBuffer.size());
	this->vertices.count = static_cast<uint32_t>(vertexBuffer.size());


	//staging buffer
	//vrt::Buffer stagingBuffer;

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
	vertices.buffer.bufferData.bufferName = "gltfModelVerticesBuffer";
	vertices.buffer.bufferData.bufferMemoryName = "gltfModelVerticesBufferMemory";

	//created vertices buffer
	if (pCoreBase->CreateBuffer(
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
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

	copyRegion.size = indexBufferSize;
	vkCmdCopyBuffer(cmdBuffer, indexStaging.bufferData.buffer, indices.buffer.bufferData.buffer, 1, &copyRegion);

	//submit temporary command buffer and destroy command buffer/memory
	pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

	//destroy staging buffers
	vkDestroyBuffer(pCoreBase->devices.logical, vertexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, vertexStaging.bufferData.memory, nullptr);
	vkDestroyBuffer(pCoreBase->devices.logical, indexStaging.bufferData.buffer, nullptr);
	vkFreeMemory(pCoreBase->devices.logical, indexStaging.bufferData.memory, nullptr);

	//output end function for console readability
	//std::cout << "\n------------------------------------------------------------------\n" << std::endl;
	//std::cout << "//\t\t\tEnd of LoadglTFFile\t\t\t//" << std::endl;
	//std::cout << "\n------------------------------------------------------------------\n" << std::endl;
}

glm::mat4 glTF_model::Node::GetLocalMatrix() {
	return (glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale)) * matrix;
}
