#pragma once

#include <EngineCore.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>

namespace gtp {

class Sphere {
public:

	//ctor
	Sphere();

	//init ctor
	Sphere(EngineCore* engineCorePtr);
	
	//returns sphere vertices
	std::vector<gtp::Model::Vertex> SphereVertices();

	//returns sphere indices
	std::vector<uint32_t> SphereIndices();

	void DestroySphere();

private:
	std::vector<gtp::Model::Vertex> sphereVertices;
	std::vector<uint32_t> sphereIndices;

	//core pointer
	EngineCore* pEngineCore = nullptr;

	//init func
	void InitSphere(EngineCore* engineCorePtr);

	void InitSphereGeometry();


};

}
