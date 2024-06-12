#pragma once

#include <TFMesh.hpp>

// -- Node Class
class TFNode {
public:

	struct Skin {
		// -- skin name
		std::string name;

		// -- skeleton root node pointer
		TFNode* skeletonRoot = nullptr;

		// -- inverse bind matrices
		std::vector<glm::mat4> inverseBindMatrices;

		// -- joints
		std::vector<TFNode*> joints;

		// -- ssbo
		vrt::Buffer ssbo;

		// -- descriptor
		VkDescriptorSet descriptorSet{};

	};

	//skin pointer
	Skin* skin{};

	// -- node index
	uint32_t index = -1;
	// -- node name
	std::string name = " ";
	// -- skin index
	int32_t skinIndex = -1;
	// -- mesh flag
	bool hasMesh = false;

	// -- parent node pointer
	TFNode* parentNode = nullptr;

	// -- children node pointers
	std::vector<TFNode*> children;

	// -- matrices data
	glm::mat4 matrix;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};

	// -- mesh class decl.
	TFMesh mesh;
	
	// -- skin class decl.
	//Skin* skin;
	
	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- ctor
	TFNode();

	// -- init ctor
	TFNode(CoreBase* coreBase);

	// -- init func
	void InitTFNode(CoreBase* coreBase);
	
	// -- local matrix
	glm::mat4 LocalMatrix();

	// -- get matrix
	glm::mat4 GetMatrix();

	// -- update
	void Update();

};

