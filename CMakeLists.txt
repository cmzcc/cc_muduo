cmake_minimum_required(VERSION 3.10)
project(cc_muduo LANGUAGES CXX)

# 设置输出目录并确保其存在
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
file(MAKE_DIRECTORY ${LIBRARY_OUTPUT_DIRECTORY})

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 收集源文件（推荐显式列出）
file(GLOB SRC_LIST "*.cpp")

# 查找依赖库
find_package(Threads REQUIRED)

# 生成动态库
add_library(cc_muduo SHARED ${SRC_LIST})

# 设置包含目录和链接库
target_include_directories(cc_muduo PUBLIC include)
target_link_libraries(cc_muduo PRIVATE Threads::Threads)

# 调试模式下添加-g
target_compile_options(cc_muduo PRIVATE $<$<CONFIG:Debug>:-g>)