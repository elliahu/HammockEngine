# Collect all source and header files
file(GLOB_RECURSE SOURCE_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Add the executable
add_executable(apps
        ${SOURCE_FILES}
)

# Link the engine library
target_link_libraries(apps PRIVATE hammock)
target_include_directories(apps PRIVATE ${PROJECT_SOURCE_DIR}/include)