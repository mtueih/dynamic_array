#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <attrs.h>
#include <errno.h>
#include <stddef.h>

#if !defined(__STDC_VERSION__) || (defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L)
#include <stdbool.h>
#endif

// ADT 类型别名声明。
typedef struct dynamic_array darr_adt;

// 状态码。
enum {
	// 成功。
	DARR_SUCCESS = 0,
	// 内存分配失败。
	DARR_MEMORY_ALLOC_FAILED = ENOMEM,
	// 无效参数。
	DARR_INVALID_PARAM = EINVAL,
	// 计算溢出。
	DARR_OVERFLOW = ERANGE,
	// 未知错误。
	DARR_UNKNOWN_ERROR,
};


// API 函数原型（声明）。
// 创建、销毁、清空。
/**
 * 创建一个「动态数组」。
 * @param element_size 「动态数组」的元素大小，不能为 0。
 * @param length 「动态数组」的初始长度。
 * @return 所创建的「动态数组」指针，如果创建失败则返回「空指针」。
 */
ATTRS_NODISCARD_SIMPLE
darr_adt *darr_create(
	size_t element_size,
	size_t length
);

/**
 * 销毁一个「动态数组」。
 * @param darr 目标「动态数组」的指针。
 */
void darr_destroy(
	darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * 清空一个「动态数组」。
 * @param darr 目标「动态数组」的指针。
 */
void darr_clear(
	darr_adt *darr
) ATTRS_NONNULL(1);


// 属性获取与设置，以及元素访问。
/**
 * 获取一个「动态数组」的内部「C 数组」指针（非 const）。
 * @param darr 目标「动态数组」的指针。
 * @return 所获取的「C 数组」指针。
 */
void *darr_carr(
	darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * 获取一个「动态数组」的某个元素的指针（非 const）。
 * @param darr 目标「动态数组」的指针。
 * @param index 目标元素的索引。
 * @return 所获取的元素指针。
 */
void *darr_at(
	darr_adt *darr,
	size_t index
) ATTRS_NONNULL(1);

/**
 * 获取一个「动态数组」的元素大小。
 * @param darr 目标「动态数组」的指针。
 * @return 所获取的元素大小。
 */
size_t darr_element_size(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * 获取一个「动态数组」的长度。
 * @param darr 目标「动态数组」的指针。
 * @return 所获取的长度。
 */
size_t darr_length(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * 判断一个「动态数组」是否是空数组。
 * @param darr 目标「动态数组」的指针。
 * @return 如果目标「动态数组」是空数组则返回 true，否则返回 false。
 */
bool darr_is_empty(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * 获取一个「动态数组」的容量。
 * @param darr 目标「动态数组」的指针。
 * @return 所获取的容量。
 */
size_t darr_capacity(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * 设置一个「动态数组」的容量。
 * @warning 当「目标容量」小于「当前长度」时，「当前内容」将被截断。
 * @param darr 目标「动态数组」的指针。
 * @param new_capacity 目标容量。
 * @return 全局状态码。
 */
int darr_set_capacity(
	darr_adt *darr,
	size_t new_capacity
) ATTRS_NONNULL(1);

/**
 * 预留一个「动态数组」的容量。
 * @note 不同于 darr_set_capacity，当「目标容量」小于「当前长度」时，「当前内容」不会被截断。
 * @param darr 目标「动态数组」的指针。
 * @param new_capacity 目标容量。
 * @return 全局状态码。
 */
int darr_reserve(
	darr_adt *darr,
	size_t new_capacity
) ATTRS_NONNULL(1);

/**
 * 调整一个「动态数组」的容量到刚合适。
 * @param darr 目标「动态数组」的指针。
 * @return 全局状态码。
 */
void darr_shrink_to_fit(
	darr_adt *darr
) ATTRS_NONNULL(1);


// 增减元素。
/**
 * 追加一个元素到一个「动态数组」末尾。
 * @param darr 目标「动态数组」的指针。
 * @param element 被追加元素指针。
 * @return 全局状态码。
 */
int darr_append(
	darr_adt *darr,
	const void *element
) ATTRS_NONNULL(1, 2);

/**
 * 追加多个元素到一个「动态数组」末尾。
 * @param darr 目标「动态数组」的指针。
 * @param elements 被追加元素起始指针。
 * @param count 追加元素个数。
 * @return 全局状态码。
 */
int darr_append_n(
	darr_adt *darr,
	const void *elements,
	size_t count
) ATTRS_NONNULL(1, 2);

/**
 * 追加一个元素到一个「动态数组」开头。
 * @param darr 目标「动态数组」的指针。
 * @param element 被追加元素指针。
 * @return 全局状态码。
 */
int darr_prepend(
	darr_adt *darr,
	const void *element
) ATTRS_NONNULL(1, 2);

/**
 * 追加多个元素到一个「动态数组」开头。
 * @param darr 目标「动态数组」的指针。
 * @param elements 被追加元素起始指针。
 * @param count 追加元素个数。
 * @return 全局状态码。
 */
int darr_prepend_n(
	darr_adt *darr,
	const void *elements,
	size_t count
) ATTRS_NONNULL(1, 2);

/**
 * 插入一个元素到一个「动态数组」。
 * @param darr 目标「动态数组」的指针。
 * @param index 插入位置索引。
 * @param element 被插入元素指针。
 * @return 全局状态码。
 */
int darr_insert(
	darr_adt *darr,
	size_t index,
	const void *element
) ATTRS_NONNULL(1, 3);

/**
 * 插入多个元素到一个「动态数组」。
 * @param darr 目标「动态数组」的指针。
 * @param index 插入位置索引。
 * @param elements 被插入元素起始指针。
 * @param count 插入元素个数。
 * @return 全局状态码。
 */
int darr_insert_n(
	darr_adt *darr,
	size_t index,
	const void *elements,
	size_t count
) ATTRS_NONNULL(1, 3);

/**
 * 删除一个「动态数组」中的某个元素。
 * @param darr 目标「动态数组」的指针。
 * @param index 被删除元素的位置索引。
 */
void darr_remove(
	darr_adt *darr,
	size_t index
) ATTRS_NONNULL(1);

/**
 * 删除一个「动态数组」中的多个元素。
 * @param darr 目标「动态数组」的指针。
 * @param index 被删除元素的起始位置索引。
 * @param count 删除元素个数，为 0 表示删除到末尾。
 */
void darr_remove_n(
	darr_adt *darr,
	size_t index,
	size_t count
) ATTRS_NONNULL(1);


// ADT 操作。
/**
 * 克隆一个「动态数组」。浅拷贝。
 * @param darr 目标「动态数组」的指针。
 * @return 克隆的「动态数组」指针，克隆失败返回空指针。
 */
darr_adt *darr_clone(
	const darr_adt *darr
) ATTRS_NONNULL(1);


// 遍历。
/**
 * 遍历一个「动态数组」。
 * @param darr 目标「动态数组」的指针。
 * @param func 遍历每个元素时执行的函数的指针。
 */
void darr_foreach(
	darr_adt *darr,
	void (*func)(void *)
) ATTRS_NONNULL(1, 2);

// 查询。
/**
 * 判断一个「动态数组」中，是否包含与某个元素“相等”的元素。
 * @param darr 目标「动态数组」的指针。
 * @param element 目标元素的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例。
 * @return 包含则返回 true，不包含则返回 false。
 */
bool darr_contains(
	const darr_adt *darr,
	const void *element,
	int (*cmp)(const void *, const void *)
) ATTRS_NONNULL(1, 2, 3);

/**
 * 查找一个「动态数组」中，与某个元素“相等”的元素。
 * @param darr 目标「动态数组」的指针。
 * @param element 目标元素的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例。
 * @param backward 是否从后往前查找。
 * @return 查找到的元素的指针，未找到则返回空指针。
 */
void *darr_find(
	darr_adt *darr,
	const void *element,
	int (*cmp)(const void *, const void *),
	bool backward
) ATTRS_NONNULL(1, 2, 3);

/**
 * 使用二分查找法查找一个「动态数组」中，与某个元素“相等”的元素。
 * @note 数组应该是有序的。
 * @param darr 目标「动态数组」的指针。
 * @param element 目标元素的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例。
 * @param desc 数组是否是逆序排序的。
 * @return
 */
void *darr_find_binary(
	const darr_adt *darr,
	const void *element,
	int (*cmp)(const void *, const void *),
	bool desc
) ATTRS_NONNULL(1, 2, 3);

// 排序。
void darr_sort(
	darr_adt *darr,
	int (*cmp)(const void *, const void *),
	bool desc
) ATTRS_NONNULL(1, 2);


#endif // DYNAMIC_ARRAY_H
