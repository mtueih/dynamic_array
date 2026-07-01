/*
 * Copyright (C) 2026 mtueih
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


/*-----------------------------------------------------------------------------
 * 头文件包含
 *---------------------------------------------------------------------------*/
#include "dynamic_array.h"

#include <stdio.h>
#include <stdlib.h>


/*-----------------------------------------------------------------------------
 * 简易测试框架
 *---------------------------------------------------------------------------*/

/* 测试计数器 */
static int tests_passed = 0;
static int tests_failed = 0;

/* 测试宏 */
#define TEST_ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            printf("✓ PASS: %s\n", test_name); \
            tests_passed++; \
        } else { \
            printf("✗ FAIL: %s\n", test_name); \
            tests_failed++; \
        } \
    } while (0)

/* 辅助函数。 */

/* 打印整数数组内容 */
static void print_int_array(darr_adt *darr) {
	printf("[");
	for (size_t i = 0; i < darr_length(darr); i++) {
		int *val = (int*)darr_at(darr, i);
		if (val) {
			printf("%d", *val);
			if (i < darr_length(darr) - 1) printf(", ");
		}
	}
	printf("]\n");
}

/* 谓词函数：检查元素是否等于某个值 */
static bool predicate_equals(const void *elem, void *ctx) {
	int target = *(int*)ctx;
	int value = *(const int*)elem;
	return value == target;
}

/* 比较函数：用于排序 */
static int compare_ints(const void *a, const void *b, void *ctx) {
	(void)ctx;
	int va = *(const int*)a;
	int vb = *(const int*)b;
	return va - vb;
}

/* 遍历回调函数 */
static void print_element(void *elem, void *ctx) {
	(void)ctx;
	printf("%d ", *(int*)elem);
}


/*-----------------------------------------------------------------------------
 * 测试函数
 *---------------------------------------------------------------------------*/

/* 测试创建和销毁 */
static void test_create_destroy(void) {
	printf("\n=== 测试: 创建和销毁 ===\n");

	/* 测试正常创建 */
	darr_adt *darr = darr_create(sizeof(int), 10);
	TEST_ASSERT(darr != NULL, "创建动态数组成功");
	TEST_ASSERT(darr_capacity(darr) >= 10, "初始容量正确");
	TEST_ASSERT(darr_length(darr) == 0, "初始长度为0");
	TEST_ASSERT(darr_is_empty(darr), "数组为空");
	darr_destroy(darr);

	/* 测试创建空数组 */
	darr = darr_create(sizeof(int), 0);
	TEST_ASSERT(darr != NULL, "创建空数组成功");
	TEST_ASSERT(darr_capacity(darr) == 0, "空数组容量为0");
	darr_destroy(darr);

	/* 测试无效参数 */
	darr = darr_create(0, 10);
	TEST_ASSERT(darr == NULL, "元素大小为0时创建失败");
}

/* 测试追加元素 */
static void test_append(void) {
	printf("\n=== 测试: 追加元素 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 0);
	TEST_ASSERT(darr != NULL, "创建数组成功");

	/* 追加单个元素 */
	int val1 = 10, val2 = 20, val3 = 30;
	int ret = darr_append(darr, &val1);
	TEST_ASSERT(ret == DARR_SUCCESS, "追加第一个元素成功");

	ret = darr_append(darr, &val2);
	TEST_ASSERT(ret == DARR_SUCCESS, "追加第二个元素成功");

	ret = darr_append(darr, &val3);
	TEST_ASSERT(ret == DARR_SUCCESS, "追加第三个元素成功");

	TEST_ASSERT(darr_length(darr) == 3, "长度为3");

	/* 验证元素值 */
	int *elem = (int*)darr_at(darr, 0);
	TEST_ASSERT(elem != NULL && *elem == 10, "第一个元素值为10");

	elem = (int*)darr_at(darr, 1);
	TEST_ASSERT(elem != NULL && *elem == 20, "第二个元素值为20");

	elem = (int*)darr_at(darr, 2);
	TEST_ASSERT(elem != NULL && *elem == 30, "第三个元素值为30");

	/* 追加多个元素 */
	int vals[] = {40, 50, 60};
	ret = darr_append_n(darr, vals, 3);
	TEST_ASSERT(ret == DARR_SUCCESS, "追加多个元素成功");
	TEST_ASSERT(darr_length(darr) == 6, "长度为6");

	darr_destroy(darr);
}

/* 测试插入元素 */
static void test_insert(void) {
	printf("\n=== 测试: 插入元素 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);

	/* 在开头插入 */
	int val = 100;
	int ret = darr_insert(darr, 0, &val);
	TEST_ASSERT(ret == DARR_SUCCESS, "在开头插入成功");

	/* 在末尾插入 */
	val = 200;
	ret = darr_insert(darr, 1, &val);
	TEST_ASSERT(ret == DARR_SUCCESS, "在末尾插入成功");

	/* 在中间插入 */
	val = 150;
	ret = darr_insert(darr, 1, &val);
	TEST_ASSERT(ret == DARR_SUCCESS, "在中间插入成功");

	TEST_ASSERT(darr_length(darr) == 3, "长度为3");

	int *elem = (int*)darr_at(darr, 0);
	TEST_ASSERT(elem != NULL && *elem == 100, "第一个元素为100");

	elem = (int*)darr_at(darr, 1);
	TEST_ASSERT(elem != NULL && *elem == 150, "第二个元素为150");

	elem = (int*)darr_at(darr, 2);
	TEST_ASSERT(elem != NULL && *elem == 200, "第三个元素为200");

	/* 测试越界插入 */
	val = 999;
	ret = darr_insert(darr, 10, &val);
	TEST_ASSERT(ret == DARR_INVALID_PARAM, "越界插入返回错误");

	darr_destroy(darr);
}

/* 测试删除元素 */
static void test_remove(void) {
	printf("\n=== 测试: 删除元素 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);

	/* 先添加一些元素 */
	int vals[] = {10, 20, 30, 40, 50};
	darr_append_n(darr, vals, 5);

	/* 删除单个元素 */
	int ret = darr_remove(darr, 2);
	TEST_ASSERT(ret == DARR_SUCCESS, "删除索引2的元素成功");
	TEST_ASSERT(darr_length(darr) == 4, "删除后长度为4");

	int *elem = (int*)darr_at(darr, 2);
	TEST_ASSERT(elem != NULL && *elem == 40, "删除后索引2的元素为40");

	/* 删除多个元素 */
	ret = darr_remove_n(darr, 1, 2);
	TEST_ASSERT(ret == DARR_SUCCESS, "删除多个元素成功");
	TEST_ASSERT(darr_length(darr) == 2, "删除后长度为2");

	/* 测试越界删除 */
	ret = darr_remove(darr, 10);
	TEST_ASSERT(ret == DARR_INVALID_PARAM, "越界删除返回错误");

	darr_destroy(darr);
}

/* 测试容量管理 */
static void test_capacity(void) {
	printf("\n=== 测试: 容量管理 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);

	TEST_ASSERT(darr_capacity(darr) >= 10, "初始容量>=10");

	/* 设置容量 */
	int ret = darr_set_capacity(darr, 20);
	TEST_ASSERT(ret == DARR_SUCCESS, "设置容量成功");
	TEST_ASSERT(darr_capacity(darr) >= 20, "容量变为>=20");

	/* 预留容量 */
	ret = darr_reserve(darr, 50);
	TEST_ASSERT(ret == DARR_SUCCESS, "预留容量成功");
	TEST_ASSERT(darr_capacity(darr) >= 50, "容量变为>=50");

	/* 收缩到合适大小 */
	int vals[] = {1, 2, 3};
	darr_append_n(darr, vals, 3);
	darr_shrink_to_fit(darr);
	TEST_ASSERT(darr_capacity(darr) == 3, "收缩后容量等于长度");

	darr_destroy(darr);
}

/* 测试查找功能 */
static void test_find(void) {
	printf("\n=== 测试: 查找功能 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);
	int vals[] = {10, 20, 30, 40, 50};
	darr_append_n(darr, vals, 5);

	/* 查找存在的元素 */
	int target = 30;
	size_t index;
	bool found = darr_find(darr, predicate_equals, &target, &index, false);
	TEST_ASSERT(found == true, "找到元素30");
	TEST_ASSERT(index == 2, "元素30的索引为2");

	/* 查找不存在的元素 */
	target = 99;
	found = darr_find(darr, predicate_equals, &target, &index, false);
	TEST_ASSERT(found == false, "未找到元素99");

	/* 统计符合条件的元素 */
	target = 25;
	size_t count = darr_count(darr, predicate_equals, &target);
	TEST_ASSERT(count == 0, "没有元素等于25");

	target = 30;
	count = darr_count(darr, predicate_equals, &target);
	TEST_ASSERT(count == 1, "有1个元素等于30");

	darr_destroy(darr);
}

/* 测试排序和反转 */
static void test_sort_reverse(void) {
	printf("\n=== 测试: 排序和反转 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);
	int vals[] = {5, 2, 8, 1, 9, 3};
	darr_append_n(darr, vals, 6);

	/* 排序 */
	int ret = darr_sort(darr, compare_ints, NULL, false);
	TEST_ASSERT(ret == DARR_SUCCESS, "排序成功");

	/* 验证排序结果 */
	int expected[] = {1, 2, 3, 5, 8, 9};
	bool sorted_correctly = true;
	for (size_t i = 0; i < darr_length(darr); i++) {
		int *elem = (int*)darr_at(darr, i);
		if (!elem || *elem != expected[i]) {
			sorted_correctly = false;
			break;
		}
	}
	TEST_ASSERT(sorted_correctly, "排序结果正确");

	/* 反转 */
	ret = darr_reverse(darr);
	TEST_ASSERT(ret == DARR_SUCCESS, "反转成功");

	int expected_reversed[] = {9, 8, 5, 3, 2, 1};
	bool reversed_correctly = true;
	for (size_t i = 0; i < darr_length(darr); i++) {
		int *elem = (int*)darr_at(darr, i);
		if (!elem || *elem != expected_reversed[i]) {
			reversed_correctly = false;
			break;
		}
	}
	TEST_ASSERT(reversed_correctly, "反转结果正确");

	darr_destroy(darr);
}

/* 测试克隆 */
static void test_clone(void) {
	printf("\n=== 测试: 克隆 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);
	int vals[] = {10, 20, 30};
	darr_append_n(darr, vals, 3);

	darr_adt *clone = darr_clone(darr);
	TEST_ASSERT(clone != NULL, "克隆成功");
	TEST_ASSERT(darr_length(clone) == darr_length(darr), "克隆数组长度相同");
	TEST_ASSERT(darr_capacity(clone) == darr_capacity(darr), "克隆数组容量相同");

	/* 验证元素值 */
	bool elements_match = true;
	for (size_t i = 0; i < darr_length(darr); i++) {
		int *orig = (int*)darr_at(darr, i);
		int *cloned = (int*)darr_at(clone, i);
		if (!orig || !cloned || *orig != *cloned) {
			elements_match = false;
			break;
		}
	}
	TEST_ASSERT(elements_match, "克隆数组元素值相同");

	darr_destroy(darr);
	darr_destroy(clone);
}

/* 测试遍历 */
static void test_foreach(void) {
	printf("\n=== 测试: 遍历 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);
	int vals[] = {1, 2, 3, 4, 5};
	darr_append_n(darr, vals, 5);

	printf("正向遍历: ");
	darr_foreach(darr, print_element, NULL, false);
	printf("\n");

	printf("反向遍历: ");
	darr_foreach(darr, print_element, NULL, true);
	printf("\n");

	TEST_ASSERT(darr_length(darr) == 5, "遍历后数组长度不变");

	darr_destroy(darr);
}

/* 测试清空 */
static void test_clear(void) {
	printf("\n=== 测试: 清空 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);
	int vals[] = {1, 2, 3, 4, 5};
	darr_append_n(darr, vals, 5);

	TEST_ASSERT(darr_length(darr) == 5, "清空前长度为5");

	darr_clear(darr);
	TEST_ASSERT(darr_length(darr) == 0, "清空后长度为0");
	TEST_ASSERT(darr_is_empty(darr), "清空后数组为空");
	TEST_ASSERT(darr_capacity(darr) > 0, "清空后容量保持不变");

	darr_destroy(darr);
}

/* 测试交换元素 */
static void test_swap(void) {
	printf("\n=== 测试: 交换元素 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);
	int vals[] = {10, 20, 30, 40, 50};
	darr_append_n(darr, vals, 5);

	int ret = darr_swap(darr, 1, 3);
	TEST_ASSERT(ret == DARR_SUCCESS, "交换元素成功");

	int *elem = (int*)darr_at(darr, 1);
	TEST_ASSERT(elem != NULL && *elem == 40, "索引1的元素现在是40");

	elem = (int*)darr_at(darr, 3);
	TEST_ASSERT(elem != NULL && *elem == 20, "索引3的元素现在是20");

	/* 测试越界交换 */
	ret = darr_swap(darr, 0, 10);
	TEST_ASSERT(ret == DARR_INVALID_PARAM, "越界交换返回错误");

	darr_destroy(darr);
}

/* 测试 prepend */
static void test_prepend(void) {
	printf("\n=== 测试: 头部插入 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);

	int val1 = 30, val2 = 20, val3 = 10;
	darr_append(darr, &val1);
	darr_prepend(darr, &val2);
	darr_prepend(darr, &val3);

	TEST_ASSERT(darr_length(darr) == 3, "长度为3");

	int *elem = (int*)darr_at(darr, 0);
	TEST_ASSERT(elem != NULL && *elem == 10, "第一个元素为10");

	elem = (int*)darr_at(darr, 1);
	TEST_ASSERT(elem != NULL && *elem == 20, "第二个元素为20");

	elem = (int*)darr_at(darr, 2);
	TEST_ASSERT(elem != NULL && *elem == 30, "第三个元素为30");

	darr_destroy(darr);
}

/* 测试二分查找 */
static void test_binary_search(void) {
	printf("\n=== 测试: 二分查找 ===\n");

	darr_adt *darr = darr_create(sizeof(int), 10);
	int vals[] = {1, 3, 5, 7, 9, 11, 13};
	darr_append_n(darr, vals, 7);

	/* 查找存在的元素 */
	int target = 7;
	size_t index;
	bool found = darr_find_binary(darr, &target, compare_ints, NULL, &index, false);
	TEST_ASSERT(found == true, "二分查找到元素7");
	TEST_ASSERT(index == 3, "元素7的索引为3");

	/* 查找不存在的元素 */
	target = 6;
	found = darr_find_binary(darr, &target, compare_ints, NULL, &index, false);
	TEST_ASSERT(found == false, "二分查找未找到元素6");

	darr_destroy(darr);
}


/*-----------------------------------------------------------------------------
 * 主函数
 *---------------------------------------------------------------------------*/
int main(void) {
	printf("========================================\n");
	printf("   Dynamic Array 单元测试\n");
	printf("========================================\n");

	test_create_destroy();
	test_append();
	test_insert();
	test_remove();
	test_capacity();
	test_find();
	test_sort_reverse();
	test_clone();
	test_foreach();
	test_clear();
	test_swap();
	test_prepend();
	test_binary_search();

	printf("\n========================================\n");
	printf("   测试结果汇总\n");
	printf("========================================\n");
	printf("通过: %d\n", tests_passed);
	printf("失败: %d\n", tests_failed);
	printf("总计: %d\n", tests_passed + tests_failed);

	if (tests_failed == 0) {
		printf("\n✓ 所有测试通过！\n");
		return 0;
	} else {
		printf("\n✗ 存在失败的测试！\n");
		return 1;
	}
}
