# This file was automatically generated for projects
# without default 'CMakeLists.txt' file.

FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/main/*.*)

idf_component_register(
	SRCS ${app_sources}
	INCLUDE_DIRS "."
	REQUIRES	ArduUtils
				ArduWire
				M5JoystickHAT
)