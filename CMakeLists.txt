cmake_minimum_required(VERSION 3.12.4)

project (sbffi)
add_compile_definitions(NAPI_VERSION=${napi_build_version})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_definitions(-DNAPI_VERSION=${napi_build_version})
include_directories(${CMAKE_JS_INC})
file(GLOB SOURCE_FILES "src/*.c")

#find_package(DynCall REQUIRED)
add_subdirectory(deps/dyncall)
set(DynLoad_DIR ${DynCall_SOURCE_DIR}/dynload)
set(DynCall_DIR ${DynCall_SOURCE_DIR}/dyncall)
set(DynCallback_DIR ${DynCall_SOURCE_DIR}/dyncallback)
include_directories(${DynLoad_DIR})
include_directories(${DynCall_DIR})
include_directories(${DynCallback_DIR})


add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} dynload_s dyncall_s dyncallback_s)
