# ==============================================================================
# dynamic_array 单元测试
# ==============================================================================
# 仅当 BUILD_TESTING=ON 时由顶层 CMakeLists.txt 引入。

# 测试可执行文件，链接 dynamic_array
add_executable(test_dynamic_array test_dynamic_array.c)
target_link_libraries(test_dynamic_array PRIVATE dynamic_array)

# 注册到 CTest
add_test(NAME test_dynamic_array COMMAND test_dynamic_array)
