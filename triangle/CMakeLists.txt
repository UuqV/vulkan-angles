cmake_minimum_required(VERSION 3.16)

project(vk_app LANGUAGES CXX)

add_executable(vk_app
    src/main.cpp
)

target_compile_features(vk_app PUBLIC cxx_std_17)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)

target_link_libraries(vk_app PRIVATE
    glfw
    Vulkan::Vulkan
    Threads::Threads
)

set(source "${CMAKE_SOURCE_DIR}/resources")
set(destination "${CMAKE_SOURCE_DIR}/resources/shaders/compile.sh")

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
COMMAND /bin/sh ${CMAKE_SOURCE_DIR}/resources/shaders/compile.sh)

add_custom_command(TARGET ${PROJECT_NAME}
COMMAND ${CMAKE_COMMAND} -E create_symlink ${source} ${destination}
DEPENDS ${destination}
COMMENT "symbolic link resource folder from ${source} => ${destinatiorunn}")

set_target_properties(vk_app PROPERTIES
    OUTPUT_NAME triangle
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
