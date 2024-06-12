#pragma once

#include <Tools.hpp>

class CoreDebug {
public:

	static CoreDebug* pCoreDebug;

	// -- settings
	struct DebugSettings {
		//validation layer name
		std::vector<const char*> validationLayerName = { "VK_LAYER_KHRONOS_validation" };
		uint32_t instanceLayerCount = 0;
		bool validationLayerPresent = false;
		std::vector<VkLayerProperties> instanceLayerProperties{};
	};

	// -- function pointers
	struct DebugpFunctions {
		PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT = nullptr;
		PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT = nullptr;
	};

	// -- debug messenger
	struct DebugMessenger {
		//debug messenger
		VkDebugUtilsMessengerEXT debugMessenger;
		//debug messenger create info
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		//debug create info pnext
		void* debugCreatepNextChain = nullptr;
	};

	struct ObjectMap {
		std::unordered_map<uint64_t, std::string> obj;
	};


	//add 
	//@param fancy lambda function for creating arrays of objects and mapping them
	//@param Example: add([this]() { return functionThatReturnsObject(functionParameters); }, "objectName"); 
	template<typename Func>
	inline void add(Func createFunc, const std::string& name) {
		using HandleType = decltype(createFunc());
		HandleType handle = createFunc(); // Call the creation function to get the handle

		// Assuming HandleType has a member 'handle' or similar that holds the actual handle
		uint64_t handleValue = std::bit_cast<uint64_t>(handle); // Adjust as needed

		// Add error handling or return value checks if necessary
		if (handleValue != 0) {
			objectMap.obj[handleValue] = name;
			//std::cout << "\nCreated Object:\n```````````````\n" <<
			//	" Name: " << name << "\n" << " Handle: " << std::hex << handleValue << std::endl;
		}
		else {
			std::cerr << "\n Failed to create " << name << std::endl;
		}

		//std::cout << "\n" << std::dec;
	}

	//add array
	//@brief lambda function for creating arrays of objects and mapping them
	//@brief Example: ([this]() { return memberFunction(function params); }, function return type, "objectName");
	template<typename Func, typename HandleType>
	inline void addArray(Func createFunc, HandleType type, const std::string& name) {
		//call create func
		std::vector<HandleType> handles = createFunc();

		//count
		int counter = 0;

		for (const auto& handle : handles) {
			uint64_t handleValue = reinterpret_cast<uint64_t>(handle);

			// Create a modified name by appending the counter value
			std::string modifiedName = std::format("{}{}", name, counter);

			// Add error handling or return value checks if necessary
			if (handleValue != 0) {
				objectMap.obj[handleValue] = modifiedName;
				std::cout << "\nCreated Object:\n```````````````\n" <<
					" Name: " << modifiedName << "\n" << " Handle: " << std::hex << handleValue << std::endl;
			}
			else {
				std::cerr << "\n Failed to create " << modifiedName << std::endl;
			}

			//inc count
			counter++;
		}

		std::cout << "\n" << std::dec;
	}


	//structs
	DebugSettings debugSettings{};
	DebugpFunctions debugpFunctions{};
	DebugMessenger debugMessenger{};
	ObjectMap objectMap{};

	//debug callback function
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData);

	//ctor
	CoreDebug();

	//funcs
	// -- init debug create info
	void initDebugCreateInfo();
	// -- load function pointers
	void loadFunctionPointers(VkInstance instance);
	// -- create debug messenger
	VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);
	// -- enumerate properties
	bool enumerateProperties();
	// -- get validation layer names
	const char** getValidationLayerNames();
	// -- destroy
	void destroyDebug(VkInstance instance);
};
