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

FetchContent_MakeAvailable(glfw glm)

# Source files
set(SOURCES 
    "src/main.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})


target_link_libraries(${PROJECT_NAME} PUBLIC glm glfw Vulkan::Vulkan)
target_include_directories(${PROJECT_NAME} PUBLIC ${glm_SOURCE_DIR})
