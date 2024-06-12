#include "raytrace_main.hpp"

int main() {

	//engine contains core components for device/window/swapchain creation
	auto engine = std::make_unique<Engine>();

	//init
	engine->InitEngine();

	//run
	engine->Run();

	engine->Terminate();

}
