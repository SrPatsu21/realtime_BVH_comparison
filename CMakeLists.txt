cmake_minimum_required(VERSION 3.10)

# ============================================================
# Project
# ============================================================
set(PROJECT_NAME "bvh_comparison")
project(
    ${PROJECT_NAME}
    VERSION 1.0.0
    LANGUAGES C CXX
)

# ============================================================
# C++ Standard
# ============================================================
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Default build type
# ============================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# ============================================================
# Acceleration Structure Configuration
# ============================================================

option(USE_BLAS_SAH_NODE "Use SAH node layout for BLAS" OFF)
option(USE_BLAS_SAH_BUILDER "Use SAH builder for BLAS" OFF)

option(USE_TLAS_SAH_NODE "Use SAH node layout for TLAS" OFF)
option(USE_TLAS_SAH_BUILDER "Use SAH builder for TLAS" OFF)

# ============================================================
# Output directories
# ============================================================
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# ----------------------------
# Common compiler flags
# ----------------------------
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVK_PROTOTYPES")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_PROTOTYPES -static-libgcc -static-libstdc++")

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-Bstatic,--whole-archive -lpthread -Wl,--no-whole-archive")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Compiling in Release mode")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    if(UNIX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error=array-bounds")
    endif()
endif()

# ============================================================
# ---------------- CLIENT EXECUTABLE ---------------------------
# Graphics, input, UI, shaders
# ============================================================

# ============================================================
# Vulkan
# ============================================================
if(UNIX)
    find_package(Vulkan REQUIRED)
elseif(WIN32)
    add_subdirectory(lib/Vulkan-Headers)
    if(NOT DEFINED VulkanHeaders_VERSION)
        set(VulkanHeaders_VERSION "1.4.352")
    endif()
    add_subdirectory(lib/Vulkan-Loader)
endif()

# ============================================================
# GLFW
# ============================================================
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

add_subdirectory(lib/glfw)

# ============================================================
# Dear ImGui
# ============================================================
set(IMGUI_DIR lib/Dear-ImGui)

set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

# ============================================================
# GLM
# ============================================================
if(WIN32)
    add_library(glm INTERFACE)
    target_include_directories(glm INTERFACE lib/glm/glm)
else()
    find_package(glm REQUIRED)
endif()

# ============================================================
# stb_image
# ============================================================
set(STB_DIR lib/stb)
set(STB_SOURCES src/client/image/stb_image_impl.cpp)

# ============================================================
# Assimp
# ============================================================
option(ASSIMP_BUILD_TESTS OFF)
option(ASSIMP_BUILD_ASSIMP_TOOLS OFF)
option(ASSIMP_BUILD_SAMPLES OFF)

add_subdirectory(lib/assimp)

set(ASSIMP_PATH "${CMAKE_SOURCE_DIR}/lib/assimp")

target_compile_options(
    assimp
    PRIVATE
    -Wno-error=maybe-uninitialized
)

# ============================================================
# KTX (Khronos Texture)
# ============================================================
option(KTX_FEATURE_STATIC_LIBRARY ON)
option(KTX_FEATURE_TESTS OFF)
option(KTX_FEATURE_TOOLS OFF)
option(KTX_FEATURE_VK_UPLOAD ON)

add_subdirectory(lib/KTX-Software)

# ============================================================
# Shader compilation
# ============================================================
find_program(GLSLANG_VALIDATOR glslangValidator)
if(NOT GLSLANG_VALIDATOR)
    message(FATAL_ERROR "glslangValidator not found")
endif()

set(SHADER_DIR ${CMAKE_SOURCE_DIR}/src/client/assets/shaders)
set(COMPILED_SHADER_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${COMPILED_SHADER_DIR})

set(SHADERS
    triangle.frag.glsl
    triangle.vert.glsl
    particle.frag.glsl
    particle.vert.glsl
)

set(SHADER_OUTPUTS "")
foreach(SHADER_NAME IN LISTS SHADERS)
    set(SHADER ${SHADER_DIR}/${SHADER_NAME})
    set(SPV ${COMPILED_SHADER_DIR}/${SHADER_NAME}.spv)

    if(SHADER_NAME MATCHES "\\.vert\\.glsl$")
        set(STAGE vert)
    elseif(SHADER_NAME MATCHES "\\.frag\\.glsl$")
        set(STAGE frag)
    else()
        message(FATAL_ERROR "Unknown shader stage for ${SHADER_NAME}")
    endif()

    add_custom_command(
        OUTPUT ${SPV}
        COMMAND ${GLSLANG_VALIDATOR} -V --target-env vulkan1.3 -S ${STAGE} ${SHADER} -o ${SPV}
        DEPENDS ${SHADER}
    )

    list(APPEND SHADER_OUTPUTS ${SPV})
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SHADER_OUTPUTS})

# ============================================================
# Copy models (CLIENT ONLY)
# ============================================================
set(MODELS_SRC_DIR ${CMAKE_SOURCE_DIR}/src/client/assets/models)
set(MODELS_DST_DIR ${CMAKE_BINARY_DIR}/models)

add_custom_target(Models ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory ${MODELS_DST_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${MODELS_SRC_DIR}
            ${MODELS_DST_DIR}
)

# ============================================================
# Client sources & executable
# ============================================================
file(GLOB_RECURSE CLIENT_SOURCES
    src/client/*.cpp
)

add_executable(${PROJECT_NAME}_client
    ${CLIENT_SOURCES}
    ${IMGUI_SOURCES}
    ${STB_SOURCES}
)

add_dependencies(${PROJECT_NAME}_client Shaders Models)

# ============================================================
# Client include directories
# ============================================================
target_include_directories(
    ${PROJECT_NAME}_client
    PRIVATE
    src/client
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    ${STB_DIR}
    ${ASSIMP_PATH}
)

if(WIN32)
    target_include_directories(
        ${PROJECT_NAME}_client
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/Vulkan-Headers/include
    )
endif()

# ============================================================
# Client compile definitions
# ============================================================

target_compile_definitions(${PROJECT_NAME}_client PRIVATE
    VK_PROTOTYPES
)

if(USE_BLAS_SAH_NODE)
    target_compile_definitions(${PROJECT_NAME}_client PRIVATE
        USE_BLAS_SAH_NODE
    )
endif()

if(USE_BLAS_SAH_BUILDER)
    target_compile_definitions(${PROJECT_NAME}_client PRIVATE
        USE_BLAS_SAH_BUILDER
    )
endif()

if(USE_TLAS_SAH_NODE)
    target_compile_definitions(${PROJECT_NAME}_client PRIVATE
        USE_TLAS_SAH_NODE
    )
endif()

if(USE_TLAS_SAH_BUILDER)
    target_compile_definitions(${PROJECT_NAME}_client PRIVATE
        USE_TLAS_SAH_BUILDER
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(
        ${PROJECT_NAME}_client
        PRIVATE
        NDEBUG
    )
endif()

if(WIN32)
    target_compile_definitions(${PROJECT_NAME}_client PRIVATE
        VK_USE_PLATFORM_WIN32_KHR
    )
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME}_client PRIVATE
        VK_USE_PLATFORM_XCB_KHR
    )
endif()

# ============================================================
# Client libraries
# ============================================================
if(WIN32)
    target_link_directories(
        ${PROJECT_NAME}_client
        PRIVATE
        ${VULKAN_SDK_PATH}/Lib
    )

    target_link_libraries(${PROJECT_NAME}_client
        glfw
        assimp
        vulkan
        glm
        ktx
    )
elseif(UNIX)
    target_link_libraries(${PROJECT_NAME}_client
        glfw
        Vulkan::Vulkan
        glm::glm
        assimp
        ktx
        pthread
        dl
        X11
        Xrandr
        Xi
        Xcursor
    )
endif()

# ============================================================
# Output names
# ============================================================
set_target_properties(${PROJECT_NAME}_client PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_client")
