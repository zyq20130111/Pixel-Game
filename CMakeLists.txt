cmake_minimum_required(VERSION 3.20)
project(Pixel-Game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_executable(
    Pixel-Game
    src/main.cpp
    src/ThreeDUtils.cpp
    src/Camera.cpp
    src/SceneBase.cpp
    src/BigHeadSonModel.cpp
    src/PoliceModel.cpp
    src/CharacterModelBase.cpp
    src/LowPolyCharacterModel.cpp
    src/PistolBullet.cpp
    src/BulletBase.cpp
    src/Pistol.cpp
    src/WeaponBase.cpp
    src/BaseUI.cpp
    src/LoginScreen.cpp
    src/MainScene.cpp
)
target_link_libraries(Pixel-Game PRIVATE glfw OpenGL::GL)

if (MSVC)
    target_compile_options(Pixel-Game PRIVATE /W4 /permissive- /utf-8)
else()
    target_compile_options(Pixel-Game PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (WIN32)
    target_compile_definitions(Pixel-Game PRIVATE NOMINMAX)
    set_target_properties(Pixel-Game PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if (MSVC)
    target_link_options(Pixel-Game PRIVATE /ENTRY:mainCRTStartup)
endif()
