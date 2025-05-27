cmake_minimum_required(VERSION 3.27)

# Project Name
project(GP1_DirectX)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Vulkan REQUIRED)

#Fetch GLFW
include(FetchContent)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)
#Fetch GLM
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 1.0.1
)

#Fetch Assimp
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG v5.4.3
)

#Fetch tinyOBJLoader
FetchContent_Declare(
  tinyobjloader
  GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
  GIT_TAG v2.0.0rc13
)

FetchContent_Declare(
    stb_image
    URL https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    DOWNLOAD_NO_EXTRACT TRUE
)

FetchContent_MakeAvailable(glfw glm assimp tinyobjloader stb_image)

# Source files
set(SOURCES 
 "src/main.cpp"
 "src/GGVulkan.cpp" 
 "src/Model.cpp" 
 "src/Scene.cpp" 
 "src/VkErrorHandler.cpp" 
 "src/GGSwapChain.cpp" 
 "src/GGVkHelperFunctions.cpp" 
 "src/GGImage.cpp" 
 "src/GGTexture.cpp" 
 "src/GGBuffer.cpp" 
 "src/GGDescriptorManager.cpp" 
 "src/GGCommandManager.cpp" 
 "src/GGPipeline.cpp" 
 "src/GGVkDevice.cpp"
 "src/Time.cpp"
 "src/GGCamera.cpp"   "src/GGShader.h" "src/GGGBuffer.cpp")

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})


function(copyFolder ORIGINALFOLDER DESTFOLDER)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${ORIGINALFOLDER}
    ${DESTFOLDER}
)
endfunction()

copyFolder(${CMAKE_CURRENT_SOURCE_DIR}/shaders  ${CMAKE_CURRENT_BINARY_DIR}/shaders)
copyFolder(${CMAKE_CURRENT_SOURCE_DIR}/resources  ${CMAKE_CURRENT_BINARY_DIR}/resources)
# Create an interface library for stb_image
add_library(stb_image INTERFACE)

target_link_libraries(${PROJECT_NAME} PUBLIC glm glfw Vulkan::Vulkan assimp stb_image)
target_include_directories(${PROJECT_NAME} PUBLIC ${glm_SOURCE_DIR} ${stb_image_SOURCE_DIR} ${tinyobjloader_SOURCE_DIR} VULKAN_PROJ_BASE_DIR)

# Locate glslc
find_program(GLSLC_EXECUTABLE glslc HINTS ENV VULKAN_SDK PATH_SUFFIXES Bin bin)
if(NOT GLSLC_EXECUTABLE)
    message(FATAL_ERROR "glslc not found. Make sure the Vulkan SDK is installed and VULKAN_SDK is set.")
endif()

# Gather all shader files
file(GLOB_RECURSE SHADER_SOURCE_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.comp"
)

# Output directory for compiled shaders
set(SHADER_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")

# List to hold all output .spv files
set(SPIRV_BINARY_FILES)

foreach(SHADER_FILE ${SHADER_SOURCE_FILES})
    get_filename_component(FILE_NAME ${SHADER_FILE} NAME)
    set(SPV_FILE "${SHADER_BINARY_DIR}/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPV_FILE}
        COMMAND ${GLSLC_EXECUTABLE} -o ${SPV_FILE} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        VERBATIM
    )

    list(APPEND SPIRV_BINARY_FILES ${SPV_FILE})
endforeach()

# Dummy custom target that depends on all SPIR-V binaries
add_custom_target(compile_shaders ALL DEPENDS ${SPIRV_BINARY_FILES})

# Make main project depend on compiled shaders
add_dependencies(${PROJECT_NAME} compile_shaders)