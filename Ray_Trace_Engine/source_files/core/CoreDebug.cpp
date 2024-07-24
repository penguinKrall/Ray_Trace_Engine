#include "CoreDebug.hpp"

CoreDebug *CoreDebug::pCoreDebug = nullptr;

// -- debug callback
VKAPI_ATTR VkBool32 VKAPI_CALL CoreDebug::debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

  bool isVerbose = false;
  bool isInfo = false;
  bool isWarning = false;
  bool isError = false;

  switch (messageSeverity) {
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
    isVerbose = true;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
    isInfo = true;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
    isWarning = true;
    break;
  case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
    isError = true;
    break;
  default:
    break;
  }
  // print message content
  std::cout << "\nValidation Layer Message:\n-------------------------\n "
            << pCallbackData->pMessage << std::endl;

  // Print related object handle if available
  if (pCallbackData->objectCount > 0 && pCallbackData->pObjects != nullptr &&
      pCoreDebug != nullptr && (isError || isWarning)) {
    std::cout << "\nError Related Objects:\n----------------------\n"
              << std::endl;
    for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
      VkObjectType objectType = pCallbackData->pObjects[i].objectType;
      uint64_t objectHandle = pCallbackData->pObjects[i].objectHandle;

      std::cout << "Object[" << i << "]\nType: " << objectType << "\n"
                << "Handle: " << std::hex << objectHandle << std::endl;

      // check objectMap for handle from callback and output its name if found
      auto it = pCoreDebug->objectMap.obj.find(objectHandle);
      if (it != pCoreDebug->objectMap.obj.end()) {
        std::cout << "Name: " << it->second << std::endl;
      } else {
        std::cout << "Name: Not Found" << std::endl;
      }
      std::cout << "\n";
    }
  }

  // Additional code for printing queue labels, command buffer labels, etc.
  // (code omitted for brevity)

  // Reset output to default mode (decimal)
  std::cout << std::dec;

  return VK_FALSE; // Returning VK_FALSE allows the Vulkan call to continue
                   // after the callback
}

// -- ctor
CoreDebug::CoreDebug() {
  pCoreDebug = this;
  initDebugCreateInfo();
}

// -- init debug create info
void CoreDebug::initDebugCreateInfo() {
  debugMessenger.debugCreateInfo.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  debugMessenger.debugCreateInfo.flags = 0;
  debugMessenger.debugCreateInfo.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  debugMessenger.debugCreateInfo.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  debugMessenger.debugCreateInfo.pfnUserCallback = debugCallback;
  debugMessenger.debugCreateInfo.pNext = nullptr;
}

// -- load function pointers
void CoreDebug::loadFunctionPointers(VkInstance instance) {

  std::cout << std::endl
            << "Load DebugUtilsMessengerExt Function Pointers"
            << "\n'''''''''''''''''''''''''''''''''''''''''''''\n"
            << std::endl;

  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          debugpFunctions.vkCreateDebugUtilsMessengerEXT),
      instance, "vkCreateDebugUtilsMessengerEXT");

  gtp::Utilities_EngCore::loadFunctionPointer(
      reinterpret_cast<PFN_vkVoidFunction &>(
          debugpFunctions.vkDestroyDebugUtilsMessengerEXT),
      instance, "vkDestroyDebugUtilsMessengerEXT");
}

// -- create debug messenger
VkDebugUtilsMessengerEXT CoreDebug::createDebugMessenger(VkInstance instance) {

  if (debugpFunctions.vkCreateDebugUtilsMessengerEXT(
          instance, &debugMessenger.debugCreateInfo, nullptr,
          &debugMessenger.debugMessenger) != VK_SUCCESS) {
    throw std::invalid_argument(
        "Failed to create Debug Utilities Extension Messenger");
  }

  else {
    return debugMessenger.debugMessenger;
  }
}

// -- enumerate properties
bool CoreDebug::enumerateProperties() {

  // instance layer count
  vkEnumerateInstanceLayerProperties(&debugSettings.instanceLayerCount,
                                     nullptr);

  // resize
  debugSettings.instanceLayerProperties.resize(
      debugSettings.instanceLayerCount);

  // get properties
  vkEnumerateInstanceLayerProperties(
      &debugSettings.instanceLayerCount,
      debugSettings.instanceLayerProperties.data());

  //--if validation layer supported/enabled
  int nameIDX = 0;
  for (VkLayerProperties layer : debugSettings.instanceLayerProperties) {
    if (nameIDX <= debugSettings.validationLayerName.size() &&
        strcmp(layer.layerName, debugSettings.validationLayerName[nameIDX]) ==
            0) {
      debugSettings.validationLayerPresent = true;
      break;
    }
  }

  return debugSettings.validationLayerPresent;
}

// -- get validation layers
const char **CoreDebug::getValidationLayerNames() {
  return debugSettings.validationLayerName.data();
}

// -- destroy
void CoreDebug::destroyDebug(VkInstance instance) {
  debugpFunctions.vkDestroyDebugUtilsMessengerEXT(
      instance, debugMessenger.debugMessenger, nullptr);
}
