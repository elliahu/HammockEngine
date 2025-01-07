# Add the executable
add_executable(marching_cubes
        main.cpp
        MarchingCubes.h
        Renderer.cpp
        Renderer.h
        Particle.h
)

# Link the engine library
target_link_libraries(marching_cubes PRIVATE hammock)
target_include_directories(marching_cubes PRIVATE ${PROJECT_SOURCE_DIR}/include)