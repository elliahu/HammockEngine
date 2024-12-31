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

# Define engine library
add_library(hammock_engine STATIC ${ENGINE_SOURCES} ${IMGUI_SOURCES})

# Include directories for engine
target_include_directories(hammock_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(hammock_engine PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(hammock_engine PUBLIC $ENV{VULKAN_SDK}/Include)
target_include_directories(hammock_engine PUBLIC ${PROJECT_SOURCE_DIR}/external)
target_include_directories(hammock_engine PUBLIC ${PROJECT_SOURCE_DIR}/external/imgui)


if(WIN32)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_WIN32_KHR)
    target_link_libraries(hammock_engine PRIVATE Dwmapi)

elseif(UNIX AND NOT APPLE)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_XLIB_KHR)

    # Find and link X11 libraries
    find_package(X11 REQUIRED)
    target_include_directories(hammock_engine PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(hammock_engine PRIVATE ${X11_LIBRARIES})
    target_link_libraries(hammock_engine PRIVATE ${X11_LIBRARIES} X11::Xkb)
    
elseif(APPLE)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

# Link dependencies
find_package(Vulkan REQUIRED )
target_link_libraries(hammock_engine PUBLIC Vulkan::Vulkan)

