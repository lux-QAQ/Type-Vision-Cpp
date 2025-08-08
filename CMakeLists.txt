cmake_minimum_required(VERSION 3.10.0)
project(type_vision VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 26)



#导出编译.json 方便clangd使用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(include/yalantinglibs)


# 自动收集源文件
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
    "src/*.inc"
)

# 自动收集头文件
file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
    "src/*.h"       # 如果源文件目录也有头文件
    "src/*.hpp"
)

# 创建可执行文件
add_executable(type_vision ${SOURCES} ${HEADERS})

# 添加头文件搜索路径
target_include_directories(type_vision PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(type_vision PRIVATE yalantinglibs::yalantinglibs)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)