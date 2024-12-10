# Collect all source and header files
file(GLOB_RECURSE SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Add the executable
add_executable(generate_environment_maps
        ${SOURCE_FILES}
)

# Link the engine library
target_link_libraries(generate_environment_maps PRIVATE hammock_engine)