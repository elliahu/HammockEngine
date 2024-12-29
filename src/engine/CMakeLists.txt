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
    # Find required Wayland packages
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED wayland-client wayland-protocols)
    pkg_check_modules(XKBCOMMON REQUIRED xkbcommon)

    # Ensure WAYLAND_LIBRARIES, WAYLAND_INCLUDE_DIRS, and XKBCOMMON_INCLUDE_DIRS are set
    if(NOT WAYLAND_LIBRARIES OR NOT WAYLAND_INCLUDE_DIRS OR NOT XKBCOMMON_INCLUDE_DIRS)
        message(FATAL_ERROR "Failed to find required Wayland or xkbcommon libraries")
    endif()

    # Get the Wayland protocol directory
    pkg_get_variable(WAYLAND_PROTOCOLS_DIR wayland-protocols pkgdatadir)

    # Set up XDG shell protocol generation
    find_program(WAYLAND_SCANNER wayland-scanner REQUIRED)

    # Generate XDG shell protocol files
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
               ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
        COMMAND ${WAYLAND_SCANNER} client-header ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} private-code ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
        DEPENDS ${WAYLAND_PROTOCOLS_DIR}/stable/xdg-shell/xdg-shell.xml
    )

    # Generate XDG decoration protocol files
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-client-protocol.h
               ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-protocol.c
        COMMAND ${WAYLAND_SCANNER} client-header ${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-client-protocol.h
        COMMAND ${WAYLAND_SCANNER} private-code ${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-protocol.c
        DEPENDS ${WAYLAND_PROTOCOLS_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
    )

    add_custom_target(generate_wayland_protocols
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-client-protocol.h
                ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
                ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-unstable-v1-client-protocol.h
                ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-protocol.c
    )

    target_sources(hammock_engine PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/xdg-shell-protocol.c
        ${CMAKE_CURRENT_BINARY_DIR}/xdg-decoration-protocol.c
    )
    
    target_include_directories(hammock_engine PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_include_directories(hammock_engine PRIVATE ${WAYLAND_INCLUDE_DIRS})
    target_include_directories(hammock_engine PRIVATE ${XKBCOMMON_INCLUDE_DIRS})
    add_dependencies(hammock_engine generate_wayland_protocols)

    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
    target_link_libraries(hammock_engine PRIVATE ${WAYLAND_LIBRARIES} ${XKBCOMMON_LIBRARIES})

elseif(APPLE)
    target_compile_definitions(hammock_engine PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

# Link dependencies
find_package(Vulkan REQUIRED)
target_link_libraries(hammock_engine PUBLIC Vulkan::Vulkan)

