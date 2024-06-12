#pragma once

#include <EngineCore.hpp>
//#include <Raytracing.hpp>
//#include <glTFShadows.hpp>
//#include <glTFTextured.hpp>
//#include <glTFAnimation.hpp>
#include <CoreUI.hpp>
//#include <glTF_PBR.hpp>
//#include <Math_Compute.hpp>
//#include <glTF_Reflections.hpp>
//#include <Multi_Model.hpp>
//#include <Complex_Scene.hpp>
//#include <multi_blas.hpp>
//#include <Build_Scene.hpp>
#include <gltf_viewer.hpp>

namespace gtp {


	// -- engine
	//@brief contains core objects and renderers
	class Engine : private EngineCore {
	private:

		float deltaTime = 0.0f;
		float lastTime = 0.0f;
		float timer = 0.0f;

		double lastX = 0.0f;
		double lastY = 0.0f;

		double posX = 0.0f;
		double posY = 0.0f;

		// -- user input
		void userInput();

		// -- renderers
		struct Renderers {
			//Raytracing raytracing;
			//glTFShadows gltfShadows;
			//glTFTextured gltfTextured;
			//glTFAnimation gltfAnimation;
			//glTF_PBR gltfPBR;
			//Math_Compute mathCompute;
			//glTF_Reflections gltfReflections;
			//Multi_Model multiModel;
			//Complex_Scene complexScene;
			//multi_blas multiBlas;
			//Build_Scene buildScene;
			gltf_viewer gltfViewer;
		};

		Renderers renderers{};

		//UI
		CoreUI UI{};

		// -- init renderers
		//@brief passes core instance pointer to renderer/s
		void InitRenderers();

		// -- init ui
		//@brief passes core instance pointer to UI
		void InitUI();

		// -- begin graphics command buffer
		//@brief begins graphics command buffer
		void BeginGraphicsCommandBuffer(int currentFrame);

		// -- end graphics command buffer
		//@brief ends graphics command buffer
		void EndGraphicsCommandBuffer(int currentFrame);


	public:
		// -- engine constructor
		Engine engine();

		// -- init engine
		//@brief calls inherited 'initCore' func and initializes renderers
		VkResult InitEngine();

		// -- draw 
		//@brief handles update ubos, record command buffers, queue submit, and present submit
		void Draw();

		// -- run
		//@brief main render loop
		void Run();

		// -- terminate
		//@brief destroy. everything.
		void Terminate();
	};

}

