# ==============================================================================
# dynamic_array 库定义
# ==============================================================================
# 仅负责：库目标创建 + 头文件搜索路径配置。
# 安装规则、CMake 包配置等全部位于顶层 CMakeLists.txt。

# 静态库目标
add_library(${MAIN_LIB_TARGET_NAME} STATIC ${MAIN_LIB_TARGET_NAME}.c)
add_library(${MAIN_LIB_TARGET_NAME}::${MAIN_LIB_TARGET_NAME} ALIAS ${MAIN_LIB_TARGET_NAME})

target_link_libraries(${MAIN_LIB_TARGET_NAME} PRIVATE safe_calc::safe_calc)


# 头文件搜索路径
# 指定构建期和安装期的头文件导出接口，以便在源码树内构建与在下游项目中使用时都能正确找到头文件。
target_include_directories(${MAIN_LIB_TARGET_NAME} PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ------------------------------------------------------------------------
# 安装规则
# ------------------------------------------------------------------------

# 安装项目的公共头文件，以供下游使用。
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.h"
)

# 安装库文件并导出目标，以便 find_package 能够导入
#
# ${MAIN_LIB_TARGET_NAME}::${MAIN_LIB_TARGET_NAME}
#
# 目标。
# 同时通过 INCLUDES DESTINATION 指定头文件位置，使下游无需额外配置头路径。
install(TARGETS ${MAIN_LIB_TARGET_NAME}
	EXPORT ${MAIN_PACKAGE_NAME}Targets
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
