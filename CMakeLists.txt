cmake_minimum_required(VERSION 3.10)
add_compile_options(-lglfw3 -lGL -lpthread -lXi -ldl)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(main)

add_executable(CircleSkinning src/main.cpp)

target_include_directories(CircleSkinning PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

find_package(glad CONFIG REQUIRED)
target_link_libraries(CircleSkinning PRIVATE glad::glad)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(CircleSkinning PRIVATE glfw)

find_package(fmt CONFIG REQUIRED)
target_link_libraries(CircleSkinning PRIVATE fmt::fmt)

find_package(glm CONFIG REQUIRED)
target_link_libraries(CircleSkinning PRIVATE glm::glm)

find_package(Freetype REQUIRED)
target_link_libraries(CircleSkinning PRIVATE Freetype::Freetype)
