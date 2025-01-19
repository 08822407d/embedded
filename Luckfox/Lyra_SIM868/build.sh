#!/bin/bash

# 项目根目录
PROJECT_DIR=$(pwd)

# 构建目录
BUILD_DIR="${PROJECT_DIR}/build"

# 判断 build 文件夹是否存在
if [ -d "$BUILD_DIR" ]; then
    echo "Cleaning up last build ..."
    rm -rf "$BUILD_DIR"  # 删除旧的 build 文件夹
else
    echo "'build' directory not found, creat one ..."
fi

# 创建新的 build 文件夹
mkdir "$BUILD_DIR"

# 进入 build 文件夹
cd "$BUILD_DIR"

# 运行 CMake 配置
echo "CMake configuring ..."
cmake ..

# 编译项目
echo "Building ..."
make

echo "Finished build！"