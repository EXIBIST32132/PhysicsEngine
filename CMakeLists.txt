cmake_minimum_required(VERSION 3.14)
project(MinimalGameEngine)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#------------------------------------------------------------------------------
# Use CMake's FetchContent module to automatically download dependencies.
#------------------------------------------------------------------------------
include(FetchContent)

# --- GLFW ---
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG        3.3.8
)
# Disable GLFW examples, tests, and docs for a lean build.
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS    OFF CACHE INTERNAL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS     OFF CACHE INTERNAL "Build the GLFW documentation")
FetchContent_MakeAvailable(glfw)

# --- GLAD ---
FetchContent_Declare(
        glad
        GIT_REPOSITORY https://github.com/Dav1dde/glad.git
        GIT_TAG        v0.1.36
)
FetchContent_MakeAvailable(glad)

# --- GLM ---
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# --- Bullet Physics ---
FetchContent_Declare(
        bullet
        GIT_REPOSITORY https://github.com/bulletphysics/bullet3.git
        GIT_TAG        3.24
)
# Configure Bullet to build only the necessary libraries.
set(BUILD_BULLET2_DEMOS OFF CACHE INTERNAL "")
set(BUILD_CPU_DEMOS    OFF CACHE INTERNAL "")
set(BUILD_EXTRAS       OFF CACHE INTERNAL "")
set(BUILD_UNIT_TESTS   OFF CACHE INTERNAL "")
FetchContent_MakeAvailable(bullet)

# --- Dear ImGui ---
FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.89.2
)
FetchContent_MakeAvailable(imgui)

# Build Dear ImGui as a static library, including the GLFW and OpenGL3 backends.
add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp   # Added demo file for ShowDemoWindow
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${glfw_SOURCE_DIR}/include  # Ensure GLFW headers are found
)

#------------------------------------------------------------------------------
# Add the executable target.
#------------------------------------------------------------------------------
add_executable(MinimalGameEngine main.cpp)

#------------------------------------------------------------------------------
# Include directories.
#------------------------------------------------------------------------------
# GLAD: include its generated header.
target_include_directories(MinimalGameEngine PRIVATE ${glad_SOURCE_DIR}/include)

# GLM: header-only; include directory is set via FetchContent.
target_include_directories(MinimalGameEngine PRIVATE ${glm_SOURCE_DIR})

# Bullet: add its include directories.
target_include_directories(MinimalGameEngine PRIVATE ${bullet_SOURCE_DIR}/src)

#------------------------------------------------------------------------------
# Link libraries.
#------------------------------------------------------------------------------
target_link_libraries(MinimalGameEngine PRIVATE
        glfw               # from FetchContent
        glad               # from FetchContent
        glm                # header-only
        BulletDynamics
        BulletCollision
        LinearMath
        imgui              # Dear ImGui static library (with demo file included)
)

# On macOS, GLFW requires additional frameworks.
if(APPLE)
    find_library(COCOA_LIB Cocoa)
    find_library(IOKIT_LIB IOKit)
    find_library(COREVIDEO_LIB CoreVideo)
    target_link_libraries(MinimalGameEngine PRIVATE ${COCOA_LIB} ${IOKIT_LIB} ${COREVIDEO_LIB})
endif()
