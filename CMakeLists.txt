cmake_minimum_required(VERSION 3.23.2)

project(VISUALISER)

add_executable(${PROJECT_NAME} src/visualiser.cpp src/glad/glad.c src/shaders/shader.cpp)

target_link_libraries(
    ${PROJECT_NAME} glfw GL dl
)
