# dynamic_array

[![C Standard](https://img.shields.io/badge/C-C99/C11/C17/C23-blue.svg)](https://zh.cppreference.com/c)
[![CMake](https://img.shields.io/badge/CMake-3.21+-green.svg)](https://cmake.org/)
[![GitHub License](https://img.shields.io/github/license/mtueih/dynamic_array)](LICENSE)
[![CMake on multiple platforms](https://github.com/mtueih/dynamic_array/actions/workflows/cmake-multi-platform.yml/badge.svg)](https://github.com/mtueih/dynamic_array/actions/workflows/cmake-multi-platform.yml)

## 安装

**环境要求**：

- CMake 3.21 或更高版本。

**依赖**：

- [`safe_calc`](https://github.com/mtueih/safe_calc)。

```bash
# 克隆仓库。
git clone https://github.com/mtueih/dynamic_array.git
cd dynamic_array

# 创建构建目录。
mkdir build && cd build

# 配置并安装。
cmake ..
cmake --build .
cmake --install .
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
