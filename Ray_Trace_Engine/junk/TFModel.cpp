#include "TFModel.hpp"

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tiny_gltf.h>


namespace vrt {

	VkDescriptorSetLayout vrt::descriptorSetLayoutImage = VK_NULL_HANDLE;
	VkDescriptorSetLayout vrt::descriptorSetLayoutUbo = VK_NULL_HANDLE;
	uint32_t vrt::descriptorBindingFlags = vrt::DescriptorBindingFlags::ImageBaseColor;

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


	Texture* TFModel::getTexture(uint32_t index) {
		if (index < textures.size()) {
			return &textures[index];
		}
		return nullptr;
	}

	TFModel::TFModel() {
	}

	TFModel::TFModel(CoreBase* coreBase, std::string filePath) {
		InitTFModel(coreBase, filePath);
	}

	void TFModel::InitTFModel(CoreBase* coreBase, std::string filePath) {
		this->pCoreBase = coreBase;

		const uint32_t glTFLoadingFlags =
			FileLoadingFlags::PreTransformVertices | FileLoadingFlags::PreMultiplyVertexColors |
			FileLoadingFlags::FlipY;

		loadFromFile(filePath, coreBase, this->pCoreBase->queue.graphics,
			glTFLoadingFlags);

		std::cout << "\nTFMODEL LOADED\n" << std::endl;
	}

	void TFModel::loadNode(Node* parent, const tinygltf::Node& node, uint32_t nodeIndex,
		const tinygltf::Model& model, std::vector<uint32_t>& indexBuffer, std::vector<Vertex>& vertexBuffer, float globalscale) {

		Node* newNode = new Node{};
		newNode->index = nodeIndex;
		newNode->parent = parent;
		newNode->name = node.name;


		newNode->skinIndex = node.skin;
		newNode->matrix = glm::mat4(1.0f);

		if (node.mesh >= 0) {
			newNode->hasMesh = true;
			//{
			//	std::cout << "***************node.name: " << node.name << std::endl;
			//}
		}

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
		}

		// Node with children
		if (node.children.size() > 0) {
			for (auto i = 0; i < node.children.size(); i++) {

				loadNode(newNode, model.nodes[node.children[i]], node.children[i], model, indexBuffer, vertexBuffer, globalscale);
			}
		}

		// Node contains mesh data
		if (node.mesh >= 0) {

			const tinygltf::Mesh mesh = model.meshes[node.mesh];
			Mesh newMesh = Mesh(pCoreBase, newNode->matrix, mesh.name);
			newMesh.name = mesh.name;
			std::cout << "\nMesh Name: " << newMesh.name << std::endl;

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

				Primitive* newPrimitive = new Primitive(indexStart, indexCount, primitive.material > -1 ? materials[primitive.material] : materials.back());
				newPrimitive->firstVertex = vertexStart;
				newPrimitive->vertexCount = vertexCount;
				newPrimitive->setDimensions(posMin, posMax);
				newMesh.primitives.push_back(newPrimitive);

			}

			//add mesh to node
			newNode->mesh = &newMesh;

			std::cout << "buffer list pushed back newNode->mesh->uniformBuffer.ubo" << std::endl;

			bufferList.push_back(newNode->mesh->uniformBuffer.ubo);
		}

		if (parent) {
			parent->children.push_back(newNode);
			std::cout << "child node pushed back to parent" << std::endl;
		}

		else {
			nodes.push_back(*newNode);
			std::cout << "node pushed back new node" << std::endl;
		}

		linearNodes.push_back(*newNode);
		std::cout << "linear nodes pushed back new node" << std::endl;



	}

	void TFModel::loadSkins(tinygltf::Model& model) {

		for (tinygltf::Skin& source : model.skins) {
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
				const tinygltf::Accessor& accessor = model.accessors[source.inverseBindMatrices];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
				newSkin->inverseBindMatrices.resize(accessor.count);
				memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
			}

			skins.push_back(*newSkin);
		}

	}

	void TFModel::loadImages(tinygltf::Model& model, VkQueue transferQueue) {

		//model.materials.size();
		std::cout << "gltf model materials count: " << model.materials.size() << std::endl;
		//model.textures.size();
		std::cout << "gltf model textures count: " << model.textures.size() << std::endl;

		std::cout << "gltf model image count: ";

		int idx = 0;
		for (tinygltf::Image& image : model.images) {
			Texture texture = Texture(pCoreBase);

			texture.fromglTfImage(image, path, this->pCoreBase, transferQueue);
			textures.push_back(texture);
			++idx;
		}
		std::cout << idx << "\n" << std::endl;
	}

	void TFModel::loadMaterials(tinygltf::Model& model) {

		int idx = 0;

		for (tinygltf::Material& mat : model.materials) {

			Material material = Material(this->pCoreBase);

			if (mat.values.find("baseColorTexture") != mat.values.end()) {
				material.baseColorTexture = getTexture(model.textures[mat.values["baseColorTexture"].TextureIndex()].source);
				std::cout << "loaded baseColorTexture material" << std::endl;
			}

			// Metallic roughness workflow
			if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
				material.metallicRoughnessTexture = getTexture(model.textures[mat.values["metallicRoughnessTexture"].TextureIndex()].source);
				std::cout << "loaded metallicRoughnessTexture material" << std::endl;
			}

			if (mat.values.find("roughnessFactor") != mat.values.end()) {
				material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
				std::cout << "loaded roughnessFactor material" << std::endl;
			}

			if (mat.values.find("metallicFactor") != mat.values.end()) {
				material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
				std::cout << "loaded metallicFactor material" << std::endl;
			}

			if (mat.values.find("baseColorFactor") != mat.values.end()) {
				material.baseColorFactor = glm::vec4(static_cast<float>(*mat.values["baseColorFactor"].ColorFactor().data()));
				std::cout << "loaded baseColorFactor material" << std::endl;
			}

			if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
				material.normalTexture = getTexture(model.textures[mat.additionalValues["normalTexture"].TextureIndex()].source);
				std::cout << "loaded normalTexture material" << std::endl;
			}

			else {
				material.normalTexture = &emptyTexture;
			}

			if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
				material.emissiveTexture = getTexture(model.textures[mat.additionalValues["emissiveTexture"].TextureIndex()].source);
				std::cout << "loaded emissiveTexture material" << std::endl;
			}

			if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
				material.occlusionTexture = getTexture(model.textures[mat.additionalValues["occlusionTexture"].TextureIndex()].source);
				std::cout << "loaded occlusionTexture material" << std::endl;
			}

			if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {

				tinygltf::Parameter param = mat.additionalValues["alphaMode"];

				if (param.string_value == "BLEND") {
					material.alphaMode = Material::ALPHAMODE_BLEND;
					std::cout << "loaded blend material" << std::endl;
				}

				if (param.string_value == "MASK") {
					material.alphaMode = Material::ALPHAMODE_MASK;
					std::cout << "loaded mask material" << std::endl;
				}

			}

			if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
				material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
				std::cout << "loaded alphaCutoff material" << std::endl;
			}

			materials.push_back(material);
		}

		//*!@#*!*@#)!@_#!)%!(%$(!@#_!@#!(@#*!%!@_# this motherfucker down here...
		// 
		// Push a default material at the end of the list for meshes with no material assigned
		materials.push_back(Material(pCoreBase));

	}

	void TFModel::loadAnimations(tinygltf::Model& model) {

		for (tinygltf::Animation& anim : model.animations) {
			Animation animation{};
			animation.name = anim.name;
			if (anim.name.empty()) {
				animation.name = std::to_string(animations.size());
			}

			// Samplers
			for (auto& samp : anim.samplers) {
				AnimationSampler sampler{};

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
					const tinygltf::Accessor& accessor = model.accessors[samp.input];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

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
					const tinygltf::Accessor& accessor = model.accessors[samp.output];
					const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

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
						std::cout << "unknown type" << std::endl;
						break;
					}
					}
				}

				animation.samplers.push_back(sampler);
			}

			// Channels
			for (auto& source : anim.channels) {
				AnimationChannel channel{};

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
					std::cout << "weights not yet supported, skipping channel" << std::endl;
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
	}

	void TFModel::loadFromFile(std::string filename, CoreBase* coreBase, VkQueue transferQueue, uint32_t fileLoadingFlags, float scale) {

		std::cout << "\nTF MODEL FILENAME\t\t" << filename << std::endl;

		tinygltf::Model gltfModel;
		tinygltf::TinyGLTF gltfContext;

		if (fileLoadingFlags & FileLoadingFlags::DontLoadImages) {
			gltfContext.SetImageLoader(loadImageDataFuncEmpty, nullptr);
		}

		else {
			gltfContext.SetImageLoader(loadImageDataFunc, nullptr);
		}

		size_t pos = filename.find_last_of('/');
		path = filename.substr(0, pos);

		std::string error, warning;

		//this->device = device;

		bool fileLoaded = gltfContext.LoadASCIIFromFile(&gltfModel, &error, &warning, filename);

		std::vector<uint32_t> indexBuffer;
		std::vector<Vertex> vertexBuffer;

		if (fileLoaded) {
			if (!(fileLoadingFlags & FileLoadingFlags::DontLoadImages)) {
				std::cout << "loading images" << std::endl;
				loadImages(gltfModel, pCoreBase->queue.graphics);
			}

			loadMaterials(gltfModel);

			const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];

			for (size_t i = 0; i < scene.nodes.size(); i++) {

				std::cout << "LOADNOAD: " << i << std::endl;

				const tinygltf::Node node = gltfModel.nodes[scene.nodes[i]];

				//std::cout << "node.name: " << node.name << std::endl;

				loadNode(nullptr, node, scene.nodes[i], gltfModel, indexBuffer, vertexBuffer, scale);

			}

			if (gltfModel.animations.size() > 0) {
				loadAnimations(gltfModel);
			}

			loadSkins(gltfModel);

			for (auto& node : linearNodes) {

				// Assign skins
				if (node.skinIndex > -1) {
					node.skin = &skins[node.skinIndex];
				}

				// Initial pose
				if (node.mesh) {
					node.update();
				}
			}
		}

		else {
			std::cout << "sorry sir and or madame, but the gltf file has failed to load." << std::endl;
			return;
		}

		// Pre-Calculations for requested features
		if ((fileLoadingFlags & FileLoadingFlags::PreTransformVertices) ||
			(fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors) ||
			(fileLoadingFlags & FileLoadingFlags::FlipY)) {

			const bool preTransform = fileLoadingFlags & FileLoadingFlags::PreTransformVertices;
			const bool preMultiplyColor = fileLoadingFlags & FileLoadingFlags::PreMultiplyVertexColors;
			const bool flipY = fileLoadingFlags & FileLoadingFlags::FlipY;
			for (Node node : linearNodes) {
				if (node.mesh) {
					const glm::mat4 localMatrix = node.getMatrix();
					for (Primitive* primitive : node.mesh->primitives) {
						for (uint32_t i = 0; i < primitive->vertexCount; i++) {
							Vertex& vertex = vertexBuffer[primitive->firstVertex + i];
							// Pre-transform vertex positions by node-hierarchy
							if (preTransform) {
								vertex.pos = glm::vec3(localMatrix * glm::vec4(vertex.pos, 1.0f));
								vertex.normal = glm::normalize(glm::mat3(localMatrix) * vertex.normal);
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
				std::cout << "Required extension: " << extension;
				metallicRoughnessWorkflow = false;
			}
		}

		size_t vertexBufferSize = vertexBuffer.size() * sizeof(Vertex);
		size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
		indices.count = static_cast<uint32_t>(indexBuffer.size());
		vertices.count = static_cast<uint32_t>(vertexBuffer.size());

		assert((vertexBufferSize > 0) && (indexBufferSize > 0));

		//struct StagingBuffer {
		//	VkBuffer buffer;
		//	VkDeviceMemory memory;
		//} vertexStaging, indexStaging;

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
		// Vertex buffer

		vertices.verticesBuffer.bufferData.bufferName = "gltfModelVerticesBuffer";
		vertices.verticesBuffer.bufferData.bufferMemoryName = "gltfModelVerticesBufferMemory";

		if (pCoreBase->CreateBuffer(
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&vertices.verticesBuffer,
			vertexBufferSize,
			nullptr) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create gltfmodel vertices buffer");
		}


		indices.indicesBuffer.bufferData.bufferName = "gltfModelIndicesBuffer";
		indices.indicesBuffer.bufferData.bufferMemoryName = "gltfModelIndicesBufferMemory";

		// Index buffer
		if (pCoreBase->CreateBuffer(
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
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

		//submit temporary command buffer
		pCoreBase->FlushCommandBuffer(cmdBuffer, pCoreBase->queue.graphics, pCoreBase->commandPools.graphics, true);

		vkDestroyBuffer(pCoreBase->devices.logical, vertexStaging.bufferData.buffer, nullptr);
		vkFreeMemory(pCoreBase->devices.logical, vertexStaging.bufferData.memory, nullptr);
		vkDestroyBuffer(pCoreBase->devices.logical, indexStaging.bufferData.buffer, nullptr);
		vkFreeMemory(pCoreBase->devices.logical, indexStaging.bufferData.memory, nullptr);

		getSceneDimensions();

		// Setup descriptors
		uint32_t uboCount{ 0 };
		uint32_t imageCount{ 0 };

		for (auto node : linearNodes) {
			if (node.mesh) {
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
		if (vkCreateDescriptorPool(pCoreBase->devices.logical, &descriptorPoolCI, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create gltf model descriptor pool");
		}

		// Descriptors for per-node uniform buffers
		{
			// Layout is global, so only create if it hasn't already been created before
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

				//std::cout << "buffer error test" << std::endl;

				if (vkCreateDescriptorSetLayout(pCoreBase->devices.logical, &descriptorLayoutCI, nullptr, &descriptorSetLayoutUbo) != VK_SUCCESS) {
					throw std::invalid_argument("failed to create gltf model descriptor set layout(ubo)");
				}
				//std::cout << "buffer error test TWO" << std::endl;
			}

			int bufferIdx = 0;

			for (auto node : nodes) {
				if (bufferIdx < bufferList.size()) {
					prepareNodeDescriptor(&node, descriptorSetLayoutUbo, bufferIdx);
					++bufferIdx;
				}
			}
		}

		// Descriptors for per-material images
		{
			// Layout is global, so only create if it hasn't already been created before
			if (descriptorSetLayoutImage == VK_NULL_HANDLE) {

				std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};


				if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
					VkDescriptorSetLayoutBinding imageBaseColorBinding{};
					imageBaseColorBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					imageBaseColorBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
					imageBaseColorBinding.binding = static_cast<uint32_t>(setLayoutBindings.size());
					imageBaseColorBinding.descriptorCount = 1;
					setLayoutBindings.push_back(imageBaseColorBinding);
				}
				if (descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
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
				if (material.baseColorTexture != nullptr) {

					std::cout << "\ngltf texture descriptor set test 1" << std::endl;

					material.createDescriptorSet(descriptorPool, descriptorSetLayoutImage, descriptorBindingFlags);

					std::cout << "\ngltf texture descriptor set test 2" << std::endl;

				}
			}
		}
	}

	glm::mat4 Node::localMatrix() {
		return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
	}

	glm::mat4 Node::getMatrix() {
		glm::mat4 m = localMatrix();
		Node* p = parent;
		while (p) {
			m = p->localMatrix() * m;
			p = p->parent;
		}
		return m;
	}

	void Node::update() {
		if (mesh) {
			glm::mat4 m = getMatrix();
			if (skin) {
				//std::cout << "test" << std::endl;
				mesh->uniformBlock.matrix = m;
				// Update join matrices
				glm::mat4 inverseTransform = glm::inverse(m);
				for (size_t i = 0; i < skin->joints.size(); i++) {
					Node* jointNode = skin->joints[i];
					glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
					jointMat = inverseTransform * jointMat;
					mesh->uniformBlock.jointMatrix[i] = jointMat;
				}
				mesh->uniformBlock.jointcount = (float)skin->joints.size();

				memcpy(mesh->uniformBuffer.ubo.bufferData.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
			}

			else {
				//std::cout << "test2" << std::endl;
				memcpy(mesh->uniformBuffer.ubo.bufferData.mapped, &m, sizeof(glm::mat4));
				//std::cout << "test3" << std::endl;
			}
		}

		for (auto& child : children) {
			child->update();
		}
	}

	void TFModel::getNodeDimensions(Node* node, glm::vec3& min, glm::vec3& max) {
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

	void TFModel::getSceneDimensions() {
		dimensions.min = glm::vec3(FLT_MAX);
		dimensions.max = glm::vec3(-FLT_MAX);
		for (auto node : nodes) {
			getNodeDimensions(&node, dimensions.min, dimensions.max);
		}
		dimensions.size = dimensions.max - dimensions.min;
		dimensions.center = (dimensions.min + dimensions.max) / 2.0f;
		dimensions.radius = glm::distance(dimensions.min, dimensions.max) / 2.0f;
	}

	Node* TFModel::findNode(Node* parent, uint32_t index) {
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

	Node* TFModel::nodeFromIndex(uint32_t index) {
		Node* nodeFound = nullptr;
		for (auto& node : nodes) {
			nodeFound = findNode(&node, index);
			if (nodeFound) {
				break;
			}
		}
		return nodeFound;
	}

	void TFModel::prepareNodeDescriptor(vrt::Node* node, VkDescriptorSetLayout descriptorSetLayout, int descriptorIDX) {

		if (bufferList[descriptorIDX].bufferData.buffer == VK_NULL_HANDLE) {

			std::cout << "\n\n\n 0buffer with VK_NULL_HANDLE node name: " << node->name << "\n\n\n" << std::endl;
		}

		
		std::cout << "\n attempting to prepare node descriptor\n" << std::endl;

		if (node->mesh) {
			std::cout << "\n attempting to prepare node descriptor in progress1\n" << std::endl;
			if (descriptorIDX > 0) {
				//std::cout << "\n attempting to prepare node descriptor in progress2\n" << std::endl;
				//std::cout << "1prep node descriptor_node name: " << node->name << std::endl;
				//std::cout << "2prep node descriptor_buffer name: " << bufferList[descriptorIDX - 1].bufferData.bufferName << std::endl;
				//std::cout << "3prep node descriptor_descriptorIDX - 1: " << descriptorIDX - 1 << std::endl;
				//std::cout << "4prep node descriptor_bufferList.size " << bufferList.size() << std::endl;

				if (bufferList[descriptorIDX - 1].bufferData.buffer == VK_NULL_HANDLE) {

					//std::cout << "\n\n\n 5buffer with VK_NULL_HANDLE node name: " << node->name << "\n\n\n" << std::endl;
				}

				//if (bufferList[descriptorIDX - 1].bufferData.buffer != VK_NULL_HANDLE) {

				VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
				descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				descriptorSetAllocInfo.descriptorPool = descriptorPool;
				descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
				descriptorSetAllocInfo.descriptorSetCount = 1;

				if (vkAllocateDescriptorSets(pCoreBase->devices.logical, &descriptorSetAllocInfo, &node->mesh->uniformBuffer.descriptorSet) != VK_SUCCESS) {
					throw std::invalid_argument("failed to allocate gltf model descriptor sets");
				}

				//std::cout << "buffer error test 3" << std::endl;

				node->mesh->uniformBuffer.descriptor.offset = 0;
				node->mesh->uniformBuffer.descriptor.buffer = bufferList[descriptorIDX - 1].bufferData.buffer;
				node->mesh->uniformBuffer.descriptor.range = sizeof(node->mesh->uniformBlock);

				//std::cout << "buffer error test 4" << std::endl;

				VkWriteDescriptorSet writeDescriptorSet{};
				writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				writeDescriptorSet.descriptorCount = 1;
				writeDescriptorSet.dstSet = node->mesh->uniformBuffer.descriptorSet;
				writeDescriptorSet.dstBinding = 0;
				writeDescriptorSet.pBufferInfo = &node->mesh->uniformBuffer.descriptor;

				vkUpdateDescriptorSets(pCoreBase->devices.logical, 1, &writeDescriptorSet, 0, nullptr);

				int idx = 0;

				for (auto& child : node->children) {
					if (idx < bufferList.size()) {
						prepareNodeDescriptor(child, descriptorSetLayout, idx);
						++idx;
					}
				}
			}
			//}
			//std::cout << "\n node descriptor attempt success \n" << std::endl;
		}
	}

	void TFModel::DestroyTFModel() {

		//descriptor set layouts
		vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, descriptorSetLayoutImage, nullptr);
		vkDestroyDescriptorSetLayout(this->pCoreBase->devices.logical, descriptorSetLayoutUbo, nullptr);

		//descriptor pool
		vkDestroyDescriptorPool(this->pCoreBase->devices.logical, this->descriptorPool, nullptr);

		//vertex and index buffer
		indices.indicesBuffer.destroy(this->pCoreBase->devices.logical);
		vertices.verticesBuffer.destroy(this->pCoreBase->devices.logical);

		//meshes
		//std::cout << "nodes size: " << nodes.size() << std::endl;
		//std::cout << "linear nodes size: " << linearNodes.size() << std::endl;

		for (auto& bufferlist : bufferList) {
			bufferlist.destroy(pCoreBase->devices.logical);
		}

		//textures
		int texIDX = 0;
		//std::cout << "\n\ntexIDX: " << texIDX << std::endl;
		for (auto tex : textures) {
			//std::cout << "\n\ntexIDX: " << texIDX << std::endl;
			tex.DestroyTexture();
			++texIDX;
		}

	}

	Mesh::Mesh(CoreBase* coreBase, glm::mat4 matrix, std::string newName) {

		this->pCoreBase = coreBase;
		this->uniformBlock.matrix = matrix;
		this->name = newName;

		uniformBuffer.ubo.bufferData.bufferName = this->name + "_mesh_uboBuffer";
		uniformBuffer.ubo.bufferData.bufferMemoryName = this->name + "_mesh_uboBufferMemory";

		if (pCoreBase->CreateBuffer(
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			&uniformBuffer.ubo,
			sizeof(uniformBlock),
			&uniformBlock) != VK_SUCCESS) {
			throw std::invalid_argument("failed to create mesh ubo buffer");
		}

		uniformBuffer.descriptor.buffer = uniformBuffer.ubo.bufferData.buffer;
		uniformBuffer.descriptor.offset = 0;
		uniformBuffer.descriptor.range = sizeof(uniformBlock);

		//if(vkMapMemory(pCoreB, uniformBuffer.memory, 0, sizeof(uniformBlock), 0, &uniformBuffer.mapped));
		//uniformBuffer.descriptor = { uniformBuffer.buffer, 0, sizeof(uniformBlock) };
	}

	void Primitive::setDimensions(glm::vec3 min, glm::vec3 max) {
		dimensions.min = min;
		dimensions.max = max;
		dimensions.size = max - min;
		dimensions.center = (min + max) / 2.0f;
		dimensions.radius = glm::distance(min, max) / 2.0f;
	}

	void Material::createDescriptorSet(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t descriptorBindingFlags) {
		VkDescriptorSetAllocateInfo descriptorSetAllocInfo{};
		descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSetAllocInfo.descriptorPool = descriptorPool;
		descriptorSetAllocInfo.pSetLayouts = &descriptorSetLayout;
		descriptorSetAllocInfo.descriptorSetCount = 1;

		//std::cout << "\n INSIDE gltf texture descriptor set test 1" << std::endl;

		if (vkAllocateDescriptorSets(pCoreBase->devices.logical, &descriptorSetAllocInfo, &descriptorSet) != VK_SUCCESS) {
			throw std::invalid_argument("failed to allocate gltf model MATERIAL descriptor set");
		}

		//std::cout << "\n INSIDE gltf texture descriptor set test 2" << std::endl;

		std::vector<VkDescriptorImageInfo> imageDescriptors{};
		std::vector<VkWriteDescriptorSet> writeDescriptorSets{};

		if (descriptorBindingFlags & DescriptorBindingFlags::ImageBaseColor) {
			imageDescriptors.push_back(baseColorTexture->descriptor);

			baseColorTexture->descriptor.imageLayout = baseColorTexture->imageLayout;
			baseColorTexture->descriptor.imageView = baseColorTexture->view;
			baseColorTexture->descriptor.sampler = baseColorTexture->sampler;

			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.dstSet = descriptorSet;
			writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
			writeDescriptorSet.pImageInfo = &baseColorTexture->descriptor;
			writeDescriptorSets.push_back(writeDescriptorSet);
		}

		//std::cout << "\n INSIDE gltf texture descriptor set test 3" << std::endl;

		if (normalTexture && descriptorBindingFlags & DescriptorBindingFlags::ImageNormalMap) {
			imageDescriptors.push_back(normalTexture->descriptor);

			normalTexture->descriptor.imageLayout = normalTexture->imageLayout;
			normalTexture->descriptor.imageView = normalTexture->view;
			normalTexture->descriptor.sampler = normalTexture->sampler;

			VkWriteDescriptorSet writeDescriptorSet{};
			writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeDescriptorSet.descriptorCount = 1;
			writeDescriptorSet.dstSet = descriptorSet;
			writeDescriptorSet.dstBinding = static_cast<uint32_t>(writeDescriptorSets.size());
			writeDescriptorSet.pImageInfo = &normalTexture->descriptor;
			writeDescriptorSets.push_back(writeDescriptorSet);
		}

		//std::cout << "\n INSIDE gltf texture descriptor set test 4" << std::endl;

		vkUpdateDescriptorSets(pCoreBase->devices.logical, static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);

		//std::cout << "\n INSIDE gltf texture descriptor set test 5" << std::endl;

	}

}