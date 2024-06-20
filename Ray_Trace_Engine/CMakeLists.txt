if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:MSVCRT")
endif()

cmake_minimum_required (VERSION 3.8)

project ("Ray_Trace_Engine") 

# Vulkan toolchain
find_package(Vulkan REQUIRED)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)

#============================================================================== 
# COMPILE SHADERS
#

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders/compiled)

file(GLOB SHADERS
  ${SHADER_SOURCE_DIR}/*.vert
  ${SHADER_SOURCE_DIR}/*.frag
  ${SHADER_SOURCE_DIR}/*.comp
  ${SHADER_SOURCE_DIR}/*.geom
  ${SHADER_SOURCE_DIR}/*.tesc
  ${SHADER_SOURCE_DIR}/*.tese
  ${SHADER_SOURCE_DIR}/*.mesh
  ${SHADER_SOURCE_DIR}/*.task
  ${SHADER_SOURCE_DIR}/*.rgen
  ${SHADER_SOURCE_DIR}/*.rchit
  ${SHADER_SOURCE_DIR}/*.rahit
  ${SHADER_SOURCE_DIR}/*.rmiss)

foreach(source IN LISTS SHADERS)
  get_filename_component(FILENAME ${source} NAME)
  add_custom_command(
    COMMAND
      ${glslc_executable}
      --target-spv=spv1.5   # Specify SPIR-V version 1.5 here
      -o ${SHADER_BINARY_DIR}/${FILENAME}.spv
      ${source}
    OUTPUT ${SHADER_BINARY_DIR}/${FILENAME}.spv
    DEPENDS ${source}
    COMMENT "Compiling ${FILENAME}"
  )
  list(APPEND SPV_SHADERS ${SHADER_BINARY_DIR}/${FILENAME}.spv)
endforeach()

add_custom_target(shaders ALL DEPENDS ${SPV_SHADERS})

# Project files
include_directories(header_files)
link_directories(source_files)

# Vulkan
include_directories(dependencies/VULKAN/Include)
link_directories(dependencies/VULKAN/Lib)

# ImGui
include_directories(dependencies/imgui)
link_directories(dependencies/imgui)

include_directories(dependencies/imgui/misc)
link_directories(dependencies/imgui/misc)

# ImGuiZMO
include_directories(dependencies)
link_directories(dependencies)

# GLFW
include_directories(dependencies/GLFW/include)
link_directories(dependencies/GLFW/lib-vc2022)

# ASSIMP
include_directories(dependencies/ASSIMP/include)
link_directories(dependencies/ASSIMP/lib/Release)

#GLTF
include_directories(dependencies/GLTF)
link_directories(dependencies/GLTF)

#ktx
include_directories(dependencies/ktx/include)
include_directories(dependencies/ktx/other_include)
link_directories(dependencies/ktx/lib)

# ImGui Source Files
file(GLOB IMGUI_SOURCES dependencies/imgui/*.cpp)

# TinyGLTF Source Files
file(GLOB GLTF_SOURCES dependencies/GLTF/*.cpp)

# Project Files
file(GLOB SOURCES
    header_files/*.h
    header_files/*.hpp
    source_files/*.cpp
    shaders/*.vert
    shaders/*.frag
    shaders/*.comp
    shaders/*.geom
    shaders/*.tesc
    shaders/*.tese
    shaders/*.mesh
    shaders/*.task
    shaders/*.rgen
    shaders/*.rchit
    shaders/*.rahit
    shaders/*.rmiss
    shaders/*.glsl
)

# Organize source files into folders
source_group("ImGui" FILES ${IMGUI_SOURCES})
source_group("GLTF" FILES ${GLTF_SOURCES})
source_group("Shaders" FILES ${SHADERS})
source_group("Project Files\\Headers" FILES ${HEADER_FILES})
source_group("Project Files\\Sources" FILES ${SOURCE_FILES})

# Create executable target
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES} ${GLTF_SOURCES})

# Link KTX library
target_link_libraries(${PROJECT_NAME} ktx)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT 
  "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,
  $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,
  $<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET Ray_Trace_Engine PROPERTY CXX_STANDARD 20)
endif()
  
# Vulkan
target_link_libraries(Ray_Trace_Engine Vulkan::Vulkan)

# GLFW
target_link_libraries(Ray_Trace_Engine glfw3)

# ASSIMP
target_link_libraries(Ray_Trace_Engine assimp-vc143-mt)
