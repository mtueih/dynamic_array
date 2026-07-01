# =============================================================================
# dynamic_array - 项目源码目录主 CMake 脚本
# =============================================================================
# 职责：
#	1. 项目源码的构建。
#	2. 源码构建产物的安装（如果有）。
# =============================================================================


# -----------------------------------------------------------------------------
# 模块引入
# -----------------------------------------------------------------------------
# 模块：
#	1. cmake/CPM.cmake: 提供 CPM 包管理器功能，用于下载和集成第三方依赖。
# -----------------------------------------------------------------------------
include(${PROJECT_SOURCE_DIR}/cmake/CPM.cmake)


# -----------------------------------------------------------------------------
# 依赖处理
# -----------------------------------------------------------------------------
CPMAddPackage(
	NAME safe_calc
	GITHUB_REPOSITORY mtueih/safe_calc
	GIT_TAG v0.1.0
	OPTIONS "SAFE_CALC_INSTALL OFF" "BUILD_TESTING OFF"
)


# -----------------------------------------------------------------------------
# 源码构建
# -----------------------------------------------------------------------------
add_library(${MAIN_LIB_TARGET_NAME} STATIC ${MAIN_LIB_TARGET_NAME}.c)

add_library(${MAIN_PACKAGE_NAME}::${MAIN_LIB_TARGET_NAME} ALIAS ${MAIN_LIB_TARGET_NAME})

target_link_libraries(${MAIN_LIB_TARGET_NAME} PRIVATE safe_calc::safe_calc)


# 配置项目主库的头文件搜索路径。
if (NOT DYNAMIC_ARRAY_INSTALL)
	target_include_directories(${MAIN_LIB_TARGET_NAME}
		PUBLIC ${PROJECT_SOURCE_DIR}/include
	)
else ()
	# 指定构建期和安装期的头文件导出接口，以便在源码树内构建与在下游项目中使用时都能正确找到头文件。
	target_include_directories(${MAIN_LIB_TARGET_NAME} PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
	)
endif ()


# ------------------------------------------------------------------------
# 源码构建产物的安装
# ------------------------------------------------------------------------
if (DYNAMIC_ARRAY_INSTALL)
	# 安装项目的公共头文件。
	install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
		DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
		FILES_MATCHING PATTERN "*.h"
	)

	# 安装项目主库的库文件并导出目标到项目主包的目标集。
	install(TARGETS ${MAIN_LIB_TARGET_NAME}
		EXPORT ${MAIN_PACKAGE_NAME}Targets
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	)
endif ()
