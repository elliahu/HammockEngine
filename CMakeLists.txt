cmake_minimum_required(VERSION 3.29)
project(hammock)

# Set global C++ standard
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add subdirectories
add_subdirectory(src/engine)
add_subdirectory(src/app)
add_subdirectory(tools/generate_environment_maps)

# Define the executable
add_executable(hammock src/main.cpp)

# Link engine and app libraries
target_link_libraries(hammock PRIVATE hammock_engine hammock_app)