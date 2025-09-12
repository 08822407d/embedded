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


file(GLOB_RECURSE LVGL_SOURCES
     ${CMAKE_SOURCE_DIR}/Drivers/lvgl/src/*.c
)
add_library(lvgl STATIC ${LVGL_SOURCES})
# 使用 LV_CONF_INCLUDE_SIMPLE，在用户 include 路径里找 lv_conf.h
set(LVGL_ROOT ${CMAKE_SOURCE_DIR}/Drivers/lvgl)
target_include_directories(lvgl PUBLIC
    ${LVGL_ROOT}
    ${LVGL_ROOT}/src
    ${CMAKE_SOURCE_DIR}/Core/Inc
)
target_compile_definitions(lvgl PUBLIC
    LV_CONF_INCLUDE_SIMPLE=1
)

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
    ${CMAKE_SOURCE_DIR}/Drivers/lvgl
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
    lvgl
)
