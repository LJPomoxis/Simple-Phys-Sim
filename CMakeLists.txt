cmake_minimum_required(VERSION 3.15)
set(VCPKG_TARGET_TRIPLET "x64-mingw-static")
set(VCPKG_HOST_TRIPLET "x64-mingw-static")
project(MySFMLProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(VCPK_MANIFEST_MODE ON)
set(project_name "sfmlTestProj")

find_package(nlohmann_json CONFIG REQUIRED)
find_package(SFML 3 COMPONENTS Graphics Window System CONFIG REQUIRED)

add_executable(${project_name} main.cpp)
target_compile_definitions(${project_name} PRIVATE SFML_STATIC)

target_link_libraries(sfmlTestProj PRIVATE 
    nlohmann_json::nlohmann_json
    SFML::Graphics 
    SFML::Window 
    SFML::System
)