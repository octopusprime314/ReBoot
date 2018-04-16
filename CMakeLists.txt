cmake_minimum_required(VERSION 2.5)
project(ReBoot)
set(VS_STARTUP_VERSION ReBoot)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

if (NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/glfw")
    message(WARNING "Missing directory ${CMAKE_SOURCE_DIR}/libs/glfw")
endif()
if (NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/fbx-sdk")
    message(WARNING "Missing directory ${CMAKE_SOURCE_DIR}/libs/fbx-sdk")
endif()
if (NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/freeimage")
    message(WARNING "Missing directory ${CMAKE_SOURCE_DIR}/libs/freeimage")
endif()
if (NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/model")
    message(WARNING "Missing directory ${CMAKE_SOURCE_DIR}/model")
endif()
if (NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/physics")
    message(WARNING "Missing directory ${CMAKE_SOURCE_DIR}/physics")
endif()
if (NOT IS_DIRECTORY "${CMAKE_SOURCE_DIR}/shading")
    message(WARNING "Missing directory ${CMAKE_SOURCE_DIR}/shading")
endif()

include_directories("${CMAKE_SOURCE_DIR}/libs/glfw/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/fbx-sdk/include")
include_directories("${CMAKE_SOURCE_DIR}/libs/freeimage/include")
include_directories("${CMAKE_SOURCE_DIR}/model/include")
include_directories("${CMAKE_SOURCE_DIR}/physics/include")
include_directories("${CMAKE_SOURCE_DIR}/shading/include")

FILE(GLOB MODEL_HEADER_FILES ${CMAKE_SOURCE_DIR}/model/include/*.h)
FILE(GLOB PHYSICS_HEADER_FILES ${CMAKE_SOURCE_DIR}/physics/include/*.h)
FILE(GLOB SHADING_HEADER_FILES ${CMAKE_SOURCE_DIR}/shading/include/*.h)
FILE(GLOB MODEL_SRC_FILES ${CMAKE_SOURCE_DIR}/model/src/*.cpp)
FILE(GLOB PHYSICS_SRC_FILES ${CMAKE_SOURCE_DIR}/physics/src/*.cpp)
FILE(GLOB SHADING_SRC_FILES ${CMAKE_SOURCE_DIR}/shading/src/*.cpp)
FILE(GLOB GLFWPROC_SRC_FILES ${CMAKE_SOURCE_DIR}/glfw/src/*.c)

source_group("model"   FILES ${MODEL_HEADER_FILES} ${MODEL_SRC_FILES})
source_group("physics" FILES ${PHYSICS_HEADER_FILES} ${PHYSICS_SRC_FILES})
source_group("shading" FILES ${SHADING_HEADER_FILES} ${SHADING_SRC_FILES})
source_group("glfw"    FILES ${GLFWPROC_SRC_FILES})

add_executable(ReBoot
                ${MODEL_SRC_FILES}
                ${PHYSICS_SRC_FILES}
                ${SHADING_SRC_FILES}
                ${GLFWPROC_SRC_FILES}
                ${MODEL_HEADER_FILES}
                ${PHYSICS_HEADER_FILES}
                ${SHADING_HEADER_FILES})

target_compile_features(ReBoot PRIVATE cxx_range_for)

target_link_libraries(ReBoot ${CMAKE_SOURCE_DIR}/libs/FreeImage/lib/FreeImage.lib)
target_link_libraries(ReBoot ${CMAKE_SOURCE_DIR}/libs/glfw/lib/glfw3.lib)
target_link_libraries(ReBoot debug     ${CMAKE_SOURCE_DIR}/libs/fbx-sdk/lib/debug/libfbxsdk-md.lib)
target_link_libraries(ReBoot optimized ${CMAKE_SOURCE_DIR}/libs/fbx-sdk/lib/release/libfbxsdk-md.lib)