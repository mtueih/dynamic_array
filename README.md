# dynamic_array

[![C Standard](https://img.shields.io/badge/C-C99/C11/C17/C23-blue.svg)](https://zh.cppreference.com/c)
[![CMake](https://img.shields.io/badge/CMake-3.21+-green.svg)](https://cmake.org/)
[![GitHub License](https://img.shields.io/github/license/mtueih/dynamic_array)](LICENSE)
[![CMake on multiple platforms](https://github.com/mtueih/dynamic_array/actions/workflows/cmake-build-and-test.yml/badge.svg)](https://github.com/mtueih/dynamic_array/actions/workflows/cmake-build-and-test.yml)

## 安装

**环境要求**：

- CMake 3.21 或更高版本。

**依赖**（由 CMake 自身通过 [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) 处理，不用手动安装。）：

- [`safe_calc`](https://github.com/mtueih/safe_calc)。

```bash
# 克隆仓库。
git clone https://github.com/mtueih/dynamic_array.git
cd dynamic_array

# 创建构建目录。
mkdir build && cd build

# 配置并安装。
cmake .. -DDYNAMIC_ARRAY_INSTALL=ON -DBUILD_TESTING=OFF
cmake --build .
cmake --install .
```

### CPM.cmake

**环境要求**：

- [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake)。

在 CMakeLists.txt 中添加以下内容：

```cmake
include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)

CPMAddPackage(
	NAME dynamic_array
	GITHUB_REPOSITORY mtueih/dynamic_array
	GIT_TAG v1.0.0
	OPTIONS "DYNAMIC_ARRAY_INSTALL OFF" "BUILD_TESTING OFF"
)
```

## 使用

### CMake

在 `CMakeLists.txt` 中添加以下内容：

```cmake
find_package(dynamic_array REQUIRED)

target_link_libraries(your_target PRIVATE dynamic_array::dynamic_array)
```

## 许可协议

本项目采用 [GNU 通用公共许可证 v3.0](https://www.gnu.org/licenses/gpl-3.0.html) 授权——详情请参阅 [LICENSE](LICENSE) 文件。
