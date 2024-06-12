#pragma once

#include <Tools.hpp>
#include <CoreBase.hpp>


class glTF_PBR {
public:
	
	// -- core pointer
	CoreBase* pCoreBase = nullptr;

	// -- ctor
	glTF_PBR();


	// -- init ctor
	glTF_PBR(CoreBase* coreBase);

	// -- init function
	void Init_glTF_PBR(CoreBase* coreBase);





};

