# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE M5JoystickHAT_component_sources *.*)

idf_component_register(
    SRCS ${M5JoystickHAT_component_sources}
    INCLUDE_DIRS "."
    REQUIRES    ArduWire
                ArduUtils
)