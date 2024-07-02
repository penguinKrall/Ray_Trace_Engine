#pragma once

#include <EngineCore.hpp>
#include <Shader.hpp>
#include <TextureLoader.hpp>
#include <Utilities_AS.hpp>
#include <Utilities_Renderer.hpp>
#include <Utilities_UI.hpp>

class Sphere {
public:

	//ctor
	Sphere();

	//init ctor
	Sphere(EngineCore* engineCorePtr);
	
private:
	std::vector<gtp::Model::Vertex> vertices;
	std::vector<uint32_t> indices;

	//core pointer
	EngineCore* pEngineCore = nullptr;

	//init func
	void InitSphere(EngineCore* engineCorePtr);

	void InitSphereGeometry();


};

