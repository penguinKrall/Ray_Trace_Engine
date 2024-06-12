#include "TFMaterial.hpp"

namespace vrt {

	vrt::TFMaterial::TFMaterial() {

	}

	TFMaterial::TFMaterial(CoreBase* coreBase) {
		InitMaterial(coreBase);
	}

	void TFMaterial::InitMaterial(CoreBase* coreBase) {
		this->pCoreBase = coreBase;
	}



}
