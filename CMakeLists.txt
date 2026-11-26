# Optional: use this if you prefer File > Open > Folder in Visual Studio (or build on Linux/Mac).
cmake_minimum_required(VERSION 3.15)
project(LearnSmart CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB LEARNSMART_SOURCES ${CMAKE_SOURCE_DIR}/src/*.cpp)
add_executable(LearnSmart ${LEARNSMART_SOURCES})

if (MSVC)
    target_compile_options(LearnSmart PRIVATE /W3)
else()
    target_compile_options(LearnSmart PRIVATE -Wall -Wextra)
endif()
