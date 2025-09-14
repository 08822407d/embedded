cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME GFX01-LRWANnode)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)


# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)


# ── LVGL 配置：必须在 add_subdirectory(lvgl) 之前 ─────────────────────────
# 先把潜在的旧缓存清掉（很关键）
unset(LV_BUILD_CONF_DIR  CACHE)
unset(LV_BUILD_CONF_PATH CACHE)
# 指定 lv_conf.h 的确切位置（推荐用 PATH 精确到文件）
set(LV_BUILD_CONF_PATH  "${CMAKE_SOURCE_DIR}/Core/Inc/lv_conf.h" CACHE PATH "" FORCE)
# 若更喜欢指目录，也可用：
# set(LV_BUILD_CONF_DIR   "${CMAKE_SOURCE_DIR}/Core/Inc"         CACHE PATH "" FORCE)
# 关闭“自动解析 lv_conf.h 生成 CONFIG_* 变量”的 Python/pcpp 预处理（嵌入式 + Windows 更省事）
set(LV_BUILD_SET_CONFIG_OPTS        OFF CACHE BOOL "" FORCE)
# 先把可选目标关掉；以后要用时再打开
set(CONFIG_LV_BUILD_EXAMPLES        OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_DEMOS           OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL   OFF CACHE BOOL "" FORCE)
# 引入 lvgl 子项目（路径指向完整仓库根）
add_subdirectory(Drivers/lvgl)


# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${PROJECT_SOURCE_DIR}/Core/Src/user_lcd.c
    ${PROJECT_SOURCE_DIR}/Core/Src/user_lora.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    ${CMAKE_SOURCE_DIR}/Core/Inc
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
    ${CMAKE_PROJECT_NAME} PRIVATE LV_CONF_INCLUDE_SIMPLE=1
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    lvgl::lvgl
)
