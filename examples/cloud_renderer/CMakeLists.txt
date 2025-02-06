# Add the executable
add_executable(cloud_renderer
        main.cpp
        scenes/ParticipatingMediumTestScene.h
        scenes/ParticipatingMediumTestScene.h
        scenes/CloudBoundingBoxTestScene.h
        scenes/CloudBoundingBoxTestScene.cpp
        PerlinNoise3D.h
        PerlinNoise3D.cpp
        SignedDistanceField.h
        BoundingBox.h
        Noise3D.h
)

# Link the engine library
target_link_libraries(cloud_renderer PRIVATE hammock)
target_include_directories(cloud_renderer PRIVATE ${PROJECT_SOURCE_DIR}/include)