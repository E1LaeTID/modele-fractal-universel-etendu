cmake_minimum_required(VERSION 3.20)

project(GeometryProjectionBuilder LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------
# SFML 3
# ---------------------------------------------------------

set(
    SFML_DIR
    "D:/code/C++/libraries/SFML-3.1.0/lib/cmake/SFML"
)

find_package(
    SFML 3
    REQUIRED
    COMPONENTS
        Graphics
        Window
        System
)

# ---------------------------------------------------------
# JSON
# ---------------------------------------------------------

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.12.0
)

FetchContent_MakeAvailable(nlohmann_json)

# ---------------------------------------------------------
# EXECUTABLE
# ---------------------------------------------------------

add_executable(
    GeometryProjectionBuilder

    source/main.cpp
    source/Pattern.cpp
    source/PolygonGenerator.cpp
    source/Transform.cpp
    source/ContourReducer.cpp
    source/GeometryMacro.cpp
    source/ProjectionEngine.cpp
    source/Renderer.cpp
    source/RecursionReducer.cpp

    includes/Point.hpp
    includes/Segment.hpp
    includes/Pattern.hpp
    includes/PolygonGenerator.hpp
    includes/Transform.hpp
    includes/ContourReducer.hpp
    includes/GeometryMacro.hpp
    includes/ProjectionEngine.hpp
    includes/Renderer.hpp
    includes/RecursionReducer.hpp
)

# ---------------------------------------------------------
# HEADERS
# ---------------------------------------------------------

target_include_directories(
    GeometryProjectionBuilder
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/includes"
)

# ---------------------------------------------------------
# CHEMIN RACINE DU PROJET
# ---------------------------------------------------------

target_compile_definitions(
    GeometryProjectionBuilder
    PRIVATE
        PROJECT_SOURCE_DIR_PATH="${CMAKE_CURRENT_SOURCE_DIR}"
)

# ---------------------------------------------------------
# LIBRAIRIES
# ---------------------------------------------------------

target_link_libraries(
    GeometryProjectionBuilder
    PRIVATE
        SFML::Graphics
        SFML::Window
        SFML::System
        nlohmann_json::nlohmann_json
)

# ---------------------------------------------------------
# DLL SFML
# ---------------------------------------------------------

add_custom_command(
    TARGET GeometryProjectionBuilder
    POST_BUILD

    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "D:/code/C++/libraries/SFML-3.1.0/bin"
        "$<TARGET_FILE_DIR:GeometryProjectionBuilder>"
)