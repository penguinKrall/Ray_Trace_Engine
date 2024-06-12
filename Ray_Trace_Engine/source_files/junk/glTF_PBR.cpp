#include "glTF_PBR.hpp"

glTF_PBR::glTF_PBR() {
}

glTF_PBR::glTF_PBR(CoreBase* coreBase) {

	this->Init_glTF_PBR(coreBase);

}

void glTF_PBR::Init_glTF_PBR(CoreBase* coreBase){

	this->pCoreBase = coreBase;

}
