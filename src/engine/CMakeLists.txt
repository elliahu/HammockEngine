# Collect all source and header files
file(GLOB_RECURSE ENGINE_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Conditionally include the resource file for Windows
if(WIN32)
    # Add the .rc file to the source list
    list(APPEND ENGINE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/platform/win32/resources.rc)
endif()

# Add ImGui source files
file(GLOB_RECURSE IMGUI_SOURCES
        ${PROJECT_SOURCE_DIR}/external/imgui/*.cpp
)

# Include Win32 backend files if building on Windows
if(WIN32)
    list(APPEND IMGUI_SOURCES
        ${PROJECT_SOURCE_DIR}/external/imgui/backends/imgui_impl_win32.cpp
    )
endif()

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
    target_link_libraries(hammock_engine PRIVATE wayland-client)
elseif(APPLE)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

# Link dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(hammock_engine PUBLIC Vulkan::Vulkan)

