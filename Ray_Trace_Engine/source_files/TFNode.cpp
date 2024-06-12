#include "TFNode.hpp"

TFNode::TFNode() { }

TFNode::TFNode(CoreBase* coreBase) {
	InitTFNode(coreBase);
}

void TFNode::InitTFNode(CoreBase* coreBase) {
	this->pCoreBase = coreBase;
}

// -- * glm::mat4 may require MATRIX
glm::mat4 TFNode::LocalMatrix() {
	return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * glm::mat4(1.0f);
}

glm::mat4 TFNode::GetMatrix() {
	glm::mat4 m = LocalMatrix();
	TFNode *p = parentNode;
	while (p) {
		m = p->LocalMatrix() * m;
		m = glm::mat4(1.0f) * m;
		p = p->parentNode;
	}
	return m;
}

void TFNode::Update() {

	//std::cout << "updating node" << std::endl;

	if (hasMesh) {

		//std::cout << "node name: " << this->name << std::endl;
		//std::cout << "node has mesh." << std::endl;
		//std::cout << "node mesh buffer name: " << this->mesh.uniformBufferData.ubo.bufferData.bufferName << std::endl;

		glm::mat4 m = GetMatrix();

		if (skin != nullptr) {

			//std::cout << "node has skin" << std::endl;
			//std::cout << "node skin name: " << skin->name << std::endl;

			mesh.uniformBlock.matrix = m;

			// Update join matrices
			glm::mat4 inverseTransform = glm::inverse(m);
			for (size_t i = 0; i < skin->joints.size(); i++) {
				TFNode jointNode = *skin->joints[i];
				glm::mat4 jointMat = jointNode.GetMatrix() * skin->inverseBindMatrices[i];
				jointMat = inverseTransform * jointMat;
				mesh.uniformBlock.jointMatrix[i] = jointMat;
				// Update ssbo
			}

			mesh.uniformBlock.jointcount = (float)skin->joints.size();

			skin->ssbo.copyTo(&mesh.uniformBlock.jointMatrix, static_cast<uint32_t>((float)skin->joints.size() * sizeof(glm::mat4)));


			memcpy(mesh.uniformBufferData.ubo.bufferData.mapped, &mesh.uniformBlock, sizeof(mesh.uniformBlock));
			//std::cout << "skin node ubo buffer memory copied" << std::endl;

		}
		else {
			//std::cout << "node does not have skin->" << std::endl;
			memcpy(mesh.uniformBufferData.ubo.bufferData.mapped, &m, sizeof(glm::mat4));
			if (mesh.uniformBufferData.ubo.bufferData.mapped != nullptr) {
				//std::cout << "no skin node ubo buffer memory copied" << std::endl;
			}
		}
	}

	for (auto& child : children) {
		//std::cout << "test2" << std::endl;
		//std::cout << "update child node test" << std::endl;
		child->Update();
	}

	//std::cout << "test" << std::endl;

}

