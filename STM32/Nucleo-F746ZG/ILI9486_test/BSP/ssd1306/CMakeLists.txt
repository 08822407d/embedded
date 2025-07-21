# BSP/LCD/CMakeLists.txt
# ----------------------

# 1) 把目录下所有 .c 文件加入源码列表
file(GLOB SSD1306_SRCS
     "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# 2) 声明静态库
add_library(ssd1306_bsp STATIC ${SSD1306_SRCS})

# 3) 暴露头文件路径，供其他 target 使用
target_include_directories(ssd1306_bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR})

# 4) 如果驱动依赖 HAL/SPI/FMC，把对外依赖写成 PUBLIC
#    这样链接 lcd_bsp 时会自动带上 HAL
target_link_libraries(ssd1306_bsp PUBLIC stm32cubemx)  # 假设 CubeMX 生成了此 HAL 库