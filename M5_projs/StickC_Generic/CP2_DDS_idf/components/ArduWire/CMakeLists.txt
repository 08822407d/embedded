# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE ArduWire_component_sources *.*)

idf_component_register(
    SRCS ${ArduWire_component_sources}
    INCLUDE_DIRS "."
    REQUIRES driver
)