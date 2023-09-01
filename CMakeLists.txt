cmake_minimum_required(VERSION 3.26)
project(SR2)

set(CMAKE_CXX_STANDARD 20)
set(SDL2_INCLUDE_DIR C:/Users/caste/OneDrive/Documentos/SDL2-2.28.1/include)
set(SDL2_LIB_DIR C:/Users/caste/OneDrive/Documentos/SDL2-2.28.1/lib/x64)

include_directories(${SDL2_INCLUDE_DIR})
link_directories(${SDL2_LIB_DIR})

add_executable(SR2 main.cpp
        fragment.h
        color.h
        print.h
        shaders.h
        triangle.h
        uniform.h)

target_link_libraries(${PROJECT_NAME} SDL2main SDL2)

# Enable C++20 features
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/src/*.cpp"
        )

target_include_directories(${PROJECT_NAME}
        PRIVATE ${PROJECT_SOURCE_DIR}/include
        PUBLIC ${PROJECT_SOURCE_DIR}/src
        PUBLIC /opt/homebrew/Cellar/sdl2/2.28.1/include
        PUBLIC /opt/homebrew/Cellar/glm/0.9.9.8/include
        )



target_link_libraries(${PROJECT_NAME}
        ${SDL2_LIBRARIES}

        )