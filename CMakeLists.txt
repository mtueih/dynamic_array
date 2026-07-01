# =============================================================================
# dynamic_array — 项目主 CMake 脚本
# =============================================================================
# 职责：
#	1. 项目声明。
#	2. 模块引入、依赖处理、全局变量定义等初始化操作。
#	3. 项目所有的构建任务。
#	5. 包配置与安装。
# =============================================================================


# -----------------------------------------------------------------------------
# 项目声明
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.21)
project(dynamic_array VERSION 1.0.1 LANGUAGES C)


# -----------------------------------------------------------------------------
# 全局变量定义
# -----------------------------------------------------------------------------
# 变量：
#	1. MAIN_PACKAGE_NAME: 项目主包的名称。
#	2. MAIN_LIB_TARGET_NAME: 项目主库的目标名称。
#	3. DYNAMIC_ARRAY_INSTALL: 是否启用安装。
# -----------------------------------------------------------------------------
set(MAIN_PACKAGE_NAME ${PROJECT_NAME})
set(MAIN_LIB_TARGET_NAME ${PROJECT_NAME})
option(DYNAMIC_ARRAY_INSTALL "Enable installation" OFF)


# -----------------------------------------------------------------------------
# 模块引入
# -----------------------------------------------------------------------------
# 模块：
#	1. CTest: 提供测试支持并定义 BUILD_TESTING 选项。
#	2. GNUInstallDirs: 提供跨平台的标准安装目录变量，用于安装。
#	3. CMakePackageConfigHelpers: 提供生成 CMake 包配置文件、包版本文件
#		的辅助函数。
# -----------------------------------------------------------------------------
include(CTest)

if (DYNAMIC_ARRAY_INSTALL)
	include(GNUInstallDirs)
	include(CMakePackageConfigHelpers)
endif ()


# ------------------------------------------------------------------------
# 源码构建
# ------------------------------------------------------------------------
add_subdirectory(src)


# -----------------------------------------------------------------------------
# 测试构建
# -----------------------------------------------------------------------------
# 注意：可通过设置 -DBUILD_TESTING=OFF 跳过测试构建。
# -----------------------------------------------------------------------------
if (BUILD_TESTING)
	add_subdirectory(tests)
endif ()


# -----------------------------------------------------------------------------
# 包配置与安装
# -----------------------------------------------------------------------------
if (DYNAMIC_ARRAY_INSTALL)
	# 生成并安装项目目标集文件。
	install(EXPORT ${MAIN_PACKAGE_NAME}Targets
		FILE ${MAIN_PACKAGE_NAME}Targets.cmake
		NAMESPACE ${MAIN_PACKAGE_NAME}::
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MAIN_PACKAGE_NAME}
	)

	# 从模板文件生成包配置文件。
	configure_package_config_file(
		# 模板文件（cmake/${MAIN_PACKAGE_NAME}Config.cmake.in），需要手动编写。
		${PROJECT_SOURCE_DIR}/cmake/${MAIN_PACKAGE_NAME}Config.cmake.in

		${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}Config.cmake
		INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MAIN_PACKAGE_NAME}
	)

	# 生成包版本文件。
	write_basic_package_version_file(
		${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}ConfigVersion.cmake
		VERSION ${PROJECT_VERSION}
		# 兼容性设置为 SameMajorVersion，表示主版本号相同则兼容。
		COMPATIBILITY SameMajorVersion
	)

	# 将生成的包配置文件安装到标准 CMake 包目录。
	install(FILES
		${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}Config.cmake
		${PROJECT_BINARY_DIR}/${MAIN_PACKAGE_NAME}ConfigVersion.cmake
		DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${MAIN_PACKAGE_NAME}
	)
endif ()
