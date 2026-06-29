# ==============================================================================
# dynamic_array 单元测试
# ==============================================================================
# 测试目标与注册。此文件在顶层启用 BUILD_TESTING 时被包含。

set(MAIN_LIB_TEST_EXE_TARGET_NAME test_${MAIN_LIB_TARGET_NAME})

# 构建测试可执行文件并链接 ${MAIN_LIB_TARGET_NAME} 库。
add_executable(${MAIN_LIB_TEST_EXE_TARGET_NAME} ${MAIN_LIB_TEST_EXE_TARGET_NAME}.c)
target_link_libraries(${MAIN_LIB_TEST_EXE_TARGET_NAME} PRIVATE ${MAIN_LIB_TARGET_NAME})

# 将测试用例注册到 CTest，以便使用 ctest 运行。
add_test(
	NAME ${MAIN_LIB_TEST_EXE_TARGET_NAME}
	COMMAND ${MAIN_LIB_TEST_EXE_TARGET_NAME}
)
