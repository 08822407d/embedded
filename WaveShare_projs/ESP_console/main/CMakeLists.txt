idf_component_register(
    SRCS
        "console_example_main.c"
        "console_settings.c"
        "lcd_st7789.c"          # 新增文件
    INCLUDE_DIRS "."
    # REQUIRES
    #     esp_lcd                 # ST7789 驱动在这个组件里
    #     driver                  # SPI / GPIO
)
