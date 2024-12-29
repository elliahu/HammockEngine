# Collect all source and header files
file(GLOB_RECURSE APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/*.h
)

# Define app library
add_library(hammock_app STATIC ${APP_SOURCES})

# Include directories for app
target_include_directories(hammock_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Link engine library
target_link_libraries(hammock_app PUBLIC hammock_engine)
