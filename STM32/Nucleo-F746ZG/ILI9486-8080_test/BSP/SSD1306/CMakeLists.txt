cmake_minimum_required (VERSION 3.18)

# 取文件夹名字作为 target 名，保证跨工程也不会撞车
get_filename_component(_lib ${CMAKE_CURRENT_LIST_DIR} NAME)

add_library(${_lib} INTERFACE)

# 递归收集源文件；CONFIGURE_DEPENDS 能在新增/删减文件时自动重新配置
file(GLOB_RECURSE _src
	CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_LIST_DIR}/*.c"
)

# 把源文件和头文件路径发布给消费端
target_sources(${_lib} INTERFACE ${_src})
target_include_directories(${_lib} INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/>
)