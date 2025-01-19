# 设置CMake最低版本要求
cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(Lyra_SIM868)

# 设置C标准为C99
set(CMAKE_C_STANDARD 99)

# 设置项目的根目录路径
set(PROJ_DIR ${CMAKE_SOURCE_DIR})

# 设置源码目录路径
set(SRC_DIR ${PROJ_DIR}/src)

# 设置库文件目录路径
set(LIB_DIR ${PROJ_DIR}/lib)

# 设置头文件目录路径
set(INCLUDE_DIR ${PROJ_DIR}/include)

# 使用通配符获取所有的 .c 源代码文件
file(GLOB SRC_FILES ${SRC_DIR}/*.c)
file(GLOB LIB_FILES ${LIB_DIR}/*.c)

# 合并所有源文件
set(SOURCE_FILES ${SRC_FILES} ${LIB_FILES})

# 添加包含目录
include_directories(
    ${INCLUDE_DIR}    # 项目的头文件目录
    ${LIB_DIR}        # 库文件目录
    ${SRC_DIR}        # 源代码目录
)

# 创建可执行文件
add_executable(Lyra_SIM868 ${SOURCE_FILES})

# 设置可执行文件的生成路径
set_target_properties(Lyra_SIM868 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# 如果有需要的话，链接库
# target_link_libraries(Lyra_SIM868 <libraries>)