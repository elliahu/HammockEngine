# Add the executable
add_executable(cloud_renderer
        main.cpp
        CloudRenderer.cpp
        CloudRenderer.h
)

# Link the engine library
target_link_libraries(cloud_renderer PRIVATE hammock)
target_include_directories(cloud_renderer PRIVATE ${PROJECT_SOURCE_DIR}/include)