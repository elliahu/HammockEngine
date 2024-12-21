# Collect all source and header files
file(GLOB_RECURSE ENGINE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Add ImGui source files (if not already part of your ENGINE_SOURCES)
file(GLOB_RECURSE IMGUI_SOURCES
        ${PROJECT_SOURCE_DIR}/external/imgui/*.cpp
)

# Define engine library
add_library(hammock_engine STATIC ${ENGINE_SOURCES} ${IMGUI_SOURCES})

# Include directories for engine
target_include_directories(hammock_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hammock_engine PUBLIC $ENV{VULKAN_SDK}/Include)
target_include_directories(hammock_engine PUBLIC ${PROJECT_SOURCE_DIR}/external)
target_include_directories(hammock_engine PUBLIC ${PROJECT_SOURCE_DIR}/external/imgui)

# Add platform specific defines
if(WIN32)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    target_link_libraries(hammock_engine PRIVATE Dwmapi)
elseif(UNIX AND NOT APPLE)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
elseif(APPLE)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

# Link dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(hammock_engine PUBLIC Vulkan::Vulkan)

