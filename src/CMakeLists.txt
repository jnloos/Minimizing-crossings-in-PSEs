cmake_minimum_required(VERSION 3.5)
project(project)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_C_FLAGS_RELEASE "-Ofast")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast")
set(CMAKE_CXX_EXTENSIONS OFF)

# Define source files
set(SOURCE_FILES
)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/source)
include_directories(${CMAKE_SOURCE_DIR}/source/external)
include_directories(${CMAKE_SOURCE_DIR}/source/external/nlohmann)

# Add an executable for the main application
add_executable(main
        main.cpp
        ${SOURCE_FILES}
)
