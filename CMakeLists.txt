# ==============================================================================
# dynamic_array — 顶层 CMake 构建脚本
# ==============================================================================
# 职责：
#   1. 项目声明与核心模块引入
#   2. 子目录构建目标编排
#   3. 安装规则（头文件 / 库 / CMake 包配置）
#   4. 测试入口（CTest）
# ==============================================================================
cmake_minimum_required(VERSION 3.21)
project(dynamic_array
	VERSION 1.0.0
	LANGUAGES C
)


# ------------------------------------------------------------------------
# 核心模块
# ------------------------------------------------------------------------
# CTest                     — 测试框架支持
# GNUInstallDirs            — 平台标准安装路径变量（lib / bin / include）
# CMakePackageConfigHelpers — configure_package_config_file 等包配置宏
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 查找/声明依赖包
find_package(safe_calc REQUIRED)

set(MAIN_PACKAGE_NAME ${PROJECT_NAME})
set(MAIN_LIB_TARGET_NAME ${PROJECT_NAME})


# ------------------------------------------------------------------------
# 构建目标
# ------------------------------------------------------------------------
add_subdirectory(src)


# -----------------------------------------------------------------------------
# 测试
# -----------------------------------------------------------------------------
# 控制测试构建行为。仅在启用 BUILD_TESTING 时添加 tests 子目录并注册测试。
#
# 若需跳过测试构建，可在命令行或打包流程中使用 -DBUILD_TESTING=OFF。
# -----------------------------------------------------------------------------
if (BUILD_TESTING)
	add_subdirectory(tests)
endif ()


# -----------------------------------------------------------------------------
# 包配置与安装
# -----------------------------------------------------------------------------
# 生成并安装供 find_package 导入的 CMake 包配置文件，并导出构建目标。
#
# 生成的文件包括：
#
#   1. safe_calcConfig.cmake
#      包入口配置文件，用于在下游项目中定位并导入导出目标。
#
#   2. safe_calcConfigVersion.cmake
#      用于在 find_package 时进行版本兼容性检查（SameMajorVersion）。
# -----------------------------------------------------------------------------

# 安装 EXPORT 目标文件
# 下游通过 find_package(safe_calc) 即可导入 safe_calc::safe_calc 目标
install(EXPORT ${MAIN_PACKAGE_NAME}Targets
	FILE ${MAIN_PACKAGE_NAME}Targets.cmake
	NAMESPACE ${MAIN_PACKAGE_NAME}::
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MAIN_PACKAGE_NAME}
)

# 从模板生成包配置文件。
configure_package_config_file(
	${PROJECT_SOURCE_DIR}/cmake/${MAIN_PACKAGE_NAME}Config.cmake.in
	${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}Config.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MAIN_PACKAGE_NAME}
)

# 生成版本兼容性检查文件。
write_basic_package_version_file(
	${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}ConfigVersion.cmake
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

# 将生成的包配置文件安装到标准 CMake 包目录。
install(FILES
	${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}Config.cmake
	${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}ConfigVersion.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MAIN_PACKAGE_NAME}
)
