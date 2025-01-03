cmake_minimum_required(VERSION 3.29)
project(hammock)

# Set global C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add ImGui source files
file(GLOB_RECURSE IMGUI_SOURCES
        ${PROJECT_SOURCE_DIR}/external/imgui/*.cpp
)

add_subdirectory(src)
add_subdirectory(examples)
add_subdirectory(tools)

# Define engine library
add_library(hammock STATIC ${ENGINE_SOURCES} ${IMGUI_SOURCES})

# Include directories for engine
target_include_directories(hammock PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_include_directories(hammock PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(hammock PUBLIC $ENV{VULKAN_SDK}/Include)
target_include_directories(hammock PUBLIC ${PROJECT_SOURCE_DIR}/external)
target_include_directories(hammock PUBLIC ${PROJECT_SOURCE_DIR}/external/imgui)

target_sources(hammock PRIVATE ${ENGINE_SOURCES})

if(WIN32)
    target_compile_definitions(hammock PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    target_link_libraries(hammock PRIVATE Dwmapi)

elseif(UNIX AND NOT APPLE)
    target_compile_definitions(hammock PRIVATE VK_USE_PLATFORM_XLIB_KHR)

    # Find and link X11 libraries
    find_package(X11 REQUIRED)
    target_include_directories(hammock PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(hammock PRIVATE ${X11_LIBRARIES})
    target_link_libraries(hammock PRIVATE ${X11_LIBRARIES} X11::Xkb)
    
elseif(APPLE)
    target_compile_definitions(hammock PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

# Link dependencies
find_package(Vulkan REQUIRED )
target_link_libraries(hammock PUBLIC Vulkan::Vulkan)

