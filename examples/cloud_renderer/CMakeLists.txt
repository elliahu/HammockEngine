# Collect all source and header files
file(GLOB_RECURSE SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Add the executable
add_executable(cloud_renderer
        ${SOURCE_FILES}
)

# Link the engine library
target_link_libraries(cloud_renderer PRIVATE hammock)
target_include_directories(cloud_renderer PRIVATE ${PROJECT_SOURCE_DIR}/include)