#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <attrs.h>
#include <errno.h>
#include <stddef.h>

/* C23 标准移除了 stdbool.h，因此仅在 C23 以下标准时包含此文件。 */
#if !defined(__STDC_VERSION__) || (defined(__STDC_VERSION__) && __STDC_VERSION__ < 202311L)
#  include <stdbool.h>
#endif

/* ADT 类型别名声明。 */
typedef struct dynamic_array darr_adt;

/* 全局状态码。 */
enum {
	DARR_SUCCESS = 0,                  /* 成功。 */
	DARR_MEMORY_ALLOC_FAILED = ENOMEM, /* 内存分配失败。 */
	DARR_INVALID_PARAM = EINVAL,       /* 无效参数。 */
	DARR_OVERFLOW = ERANGE,            /* 计算溢出。 */
	DARR_UNKNOWN_ERROR,                /* 未知错误。 */
};


/***************************************************************
 ******************    API 函数原型（声明）。    ******************
 **************************************************************/

/* 创建、销毁、清空。 */

/**
 * @brief 创建一个「动态数组」。
 * 
 * @note 如果指定了 length 参数的值（不为 0），那么该值会被存储为一个「保底容量值」，
 * 在后续容量自动变化时，将维持不低于这个值。
 *
 * @param element_size 「动态数组」的元素大小。不能为 0。
 * @param length 「动态数组」的初始容量长度（能够存放的元素个数，以元素个数为单位）。
 *
 * @return 所创建的「动态数组」的指针，如果创建失败则返回「空指针」。
 */
ATTRS_NODISCARD_SIMPLE
darr_adt *darr_create(
	size_t element_size,
	size_t length
);

/**
 * @brief 销毁一个「动态数组」。
 *
 * @param darr 目标「动态数组」的指针。
 */
void darr_destroy(
	darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * @brief 清空一个「动态数组」。
 *
 * @note 不会立即释放内存。
 *
 * @param darr 目标「动态数组」的指针。
 */
void darr_clear(
	darr_adt *darr
) ATTRS_NONNULL(1);

/* 属性获取与设置，以及元素访问。 */

/**
 * @brief 获取一个「动态数组」的内部「C 数组」指针（非 const）。
 * 
 * @param darr 目标「动态数组」的指针。
 * 
 * @return 所获取的「C 数组」的指针（非 const）。
 */
void *darr_carr(
	darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * @brief 获取一个「动态数组」的内部「C 数组」指针（const）。
 * 
 * @param darr 目标「动态数组」的指针。
 * 
 * @return 所获取的「C 数组」的指针（const）。
 */
const void *darr_carr_const(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * @brief 获取一个「动态数组」的某个位置处的元素的指针（非 const）。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param index 目标位置的索引。
 * 
 * @return 所获取的元素的指针（非 const）。如果下标越界，则返回「空指针」。
 */
void *darr_at(
	darr_adt *darr,
	size_t index
) ATTRS_NONNULL(1);

/**
 * @brief 获取一个「动态数组」的某个位置处的元素的指针（const）。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param index 目标位置的索引。
 * 
 * @return 所获取的元素的指针（const）。如果下标越界，则返回「空指针」。
 */
const void *darr_at_const(
	const darr_adt *darr,
	size_t index
) ATTRS_NONNULL(1);

/**
 * @brief 获取一个「动态数组」的元素大小（单个元素占用的空间大小，以字节为单位）。
 * 
 * @param darr 目标「动态数组」的指针。
 * 
 * @return 所获取的元素大小。
 */
size_t darr_element_size(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * @brief 获取一个「动态数组」的长度（元素个数）。
 * 
 * @param darr 目标「动态数组」的指针。
 * 
 * @return 所获取的长度。
 */
size_t darr_length(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * @brief 判断一个「动态数组」是否是空数组。
 * 
 * @param darr 目标「动态数组」的指针。
 * 
 * @return 如果目标「动态数组」是空数组则返回 true，否则返回 false。
 */
bool darr_is_empty(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * @brief 获取一个「动态数组」的容量（能够存放的元素个数，以元素个数为单位）。
 * 
 * @param darr 目标「动态数组」的指针。
 * 
 * @return 所获取的容量。
 */
size_t darr_capacity(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/**
 * @brief 设置一个「动态数组」的容量（能够存放的元素个数，以元素个数为单位）。
 * 
 * @warning 当「目标容量」小于「当前长度」时，「当前内容」将被截断。
 * 如果不希望截断当前内容，请使用 darr_reserve。
 * @note 如果该函数成功，则会同时修改「保底容量」。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param new_capacity 目标容量。
 * 
 * @return 全局状态码。
 */
int darr_set_capacity(
	darr_adt *darr,
	size_t new_capacity
) ATTRS_NONNULL(1);

/**
 * @brief 预留一个「动态数组」的容量。
 * 
 * @note 不同于 darr_set_capacity，当「目标容量」小于「当前长度」时，「当前内容」不会被截断。
 * @note 如果该函数成功，则会同时修改「保底容量」。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param new_capacity 目标容量。
 * 
 * @return 全局状态码。
 */
int darr_reserve(
	darr_adt *darr,
	size_t new_capacity
) ATTRS_NONNULL(1);

/**
 * @brief 调整一个「动态数组」的容量到刚合适（即容量等于元素个数）。
 * 
 * @note 调用该函数的同时会取消「保底容量」。
 * 
 * @param darr 目标「动态数组」的指针。
 * 
 * @return 全局状态码。
 */
void darr_shrink_to_fit(
	darr_adt *darr
) ATTRS_NONNULL(1);

/* 增减元素。 */

/**
 * @brief 追加一个元素到一个「动态数组」末尾。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param element 被追加元素指针。
 * 
 * @return 全局状态码。
 */
int darr_append(
	darr_adt *darr,
	const void *element
) ATTRS_NONNULL(1, 2);

/**
 * @brief 追加多个元素到一个「动态数组」末尾。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param elements 被追加元素起始指针。
 * @param count 追加元素个数。
 * 
 * @return 全局状态码。
 */
int darr_append_n(
	darr_adt *darr,
	const void *elements,
	size_t count
) ATTRS_NONNULL(1, 2);

/**
 * @brief 追加一个元素到一个「动态数组」开头。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param element 被追加元素指针。
 * 
 * @return 全局状态码。
 */
int darr_prepend(
	darr_adt *darr,
	const void *element
) ATTRS_NONNULL(1, 2);

/**
 * @brief 追加多个元素到一个「动态数组」开头。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param elements 被追加元素起始指针。
 * @param count 追加元素个数。
 * 
 * @return 全局状态码。
 */
int darr_prepend_n(
	darr_adt *darr,
	const void *elements,
	size_t count
) ATTRS_NONNULL(1, 2);

/**
 * @brief 插入一个元素到一个「动态数组」的指定位置处。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param index 插入位置索引。
 * @param element 被插入元素指针。
 * 
 * @return 全局状态码。
 */
int darr_insert(
	darr_adt *darr,
	size_t index,
	const void *element
) ATTRS_NONNULL(1, 3);

/**
 * @brief 插入多个元素到一个「动态数组」的指定位置处。
 * 
 * @param darr 目标「动态数组」的指针。
 * @param index 插入位置索引。
 * @param elements 被插入元素起始指针。
 * @param count 插入元素个数。
 * 
 * @return 全局状态码。
 */
int darr_insert_n(
	darr_adt *darr,
	size_t index,
	const void *elements,
	size_t count
) ATTRS_NONNULL(1, 3);

/**
 * @brief 删除一个「动态数组」中指定位置处的某个元素。
 *
 * @param darr 目标「动态数组」的指针。
 * @param index 被删除元素的位置索引。
 */
void darr_remove(
	darr_adt *darr,
	size_t index
) ATTRS_NONNULL(1);

/**
 * @brief 删除一个「动态数组」中从指定位置处起，向后的多个元素。
 *
 * @param darr 目标「动态数组」的指针。
 * @param index 被删除元素的起始位置索引。
 * @param count 删除元素个数，为 0 表示删除到末尾。
 */
void darr_remove_n(
	darr_adt *darr,
	size_t index,
	size_t count
) ATTRS_NONNULL(1);

/* 元素操作。 */

/**
 * @brief 交换一个「动态数组」中的两个元素。
 *
 * @param darr 目标「动态数组」的指针。
 * @param index_1 第一个元素的位置索引。
 * @param index_2 第二个元素的位置索引。
 */
void darr_swap(
	darr_adt *darr,
	size_t index_1,
	size_t index_2
);

/* ADT 操作。 */

/**
 * @brief 克隆一个「动态数组」。
 *
 * @note 函数默认是浅拷贝的。
 *
 * @param darr 目标「动态数组」的指针。
 *
 * @return 克隆的「动态数组」指针，克隆失败返回空指针。
 */
darr_adt *darr_clone(
	const darr_adt *darr
) ATTRS_NONNULL(1);

/* 遍历。 */

/**
 * @brief 遍历一个「动态数组」（非 const）。
 *
 * @param darr 目标「动态数组」的指针。
 * @param func 遍历每个元素时执行的函数的指针（非 const）。
 * @param ctx 遍历函数的上下文参数。
 */
void darr_foreach(
	darr_adt *darr,
	void (*func)(void *, void *),
	void *ctx
) ATTRS_NONNULL(1, 2);

/**
 * @brief 遍历一个「动态数组」（const）。
 *
 * @param darr 目标「动态数组」的指针。
 * @param func 遍历每个元素时执行的函数的指针（const）。
 * @param ctx 遍历函数的上下文参数。
 */
void darr_foreach_const(
	const darr_adt *darr,
	void (*func)(const void *, void *),
	void *ctx
) ATTRS_NONNULL(1, 2);

/* 查询、查找。 */

/**
 * @brief 判断一个「动态数组」中，是否包含与某个元素“相等”的元素。
 *
 * @param darr 目标「动态数组」的指针。
 * @param element 被比较元素的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例
 * （两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值）。
 * @param ctx 比较函数的上下文参数。
 * @param backward 是否从后往前查找。
 *
 * @return 包含则返回 true，不包含则返回 false。
 */
bool darr_contains(
	const darr_adt *darr,
	const void *element,
	int (*cmp)(const void *, const void *, void *),
	void *ctx,
	bool backward
) ATTRS_NONNULL(1, 2, 3);

/**
 * @brief 查找一个「动态数组」中，与某个元素“相等”的元素。
 *
 * @param darr 目标「动态数组」的指针。
 * @param element 被比较元素的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例
 * （两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值）。
 * @param ctx 比较函数的上下文参数。
 * @param out_index 输出参数，存储查找到的索引。
 * @param backward 是否从后往前查找。
 *
 * @return 找到则返回 true，未找到则返回 false。
 */
bool darr_find(
	darr_adt *darr,
	const void *element,
	int (*cmp)(const void *, const void *, void *),
	void *ctx,
	size_t *out_index,
	bool backward
) ATTRS_NONNULL(1, 2, 3);

/**
 * @brief 查找一个「动态数组」中，符合某种条件的元素。
 *
 * @param darr 目标「动态数组」的指针。
 * @param predicate 谓词函数，用于检查一个元素是否符合某种条件。
 * （两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值）。
 * @param ctx 谓词函数的上下文参数。
 * @param out_index 输出参数，存储查找到的索引。
 * @param backward 是否从后往前查找。
 *
 * @return 找到则返回 true，未找到则返回 false。
 */
bool darr_find_if(
	darr_adt *darr,
	bool (*predicate)(const void *, void *),
	void *ctx,
	size_t *out_index,
	bool backward
) ATTRS_NONNULL(1, 2, 3);

/**
 * @brief 使用二分查找法查找一个「动态数组」中，与某个元素“相等”的元素。
 *
 * @note 数组应该是有序的。
 *
 * @param darr 目标「动态数组」的指针。
 * @param element 目标元素的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例
 * （两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值）。
 * @param ctx 比较函数的上下文参数。
 * @param out_index 输出参数，存储查找到的索引。
 * @param desc 数组是否是逆序排序的。
 *
 * @return 找到则返回 true，未找到则返回 false。
 */
bool darr_find_binary(
	const darr_adt *darr,
	const void *element,
	int (*cmp)(const void *, const void *, void *),
	void *ctx,
	size_t *out_index,
	bool desc
) ATTRS_NONNULL(1, 2, 3);

/* 排序、反转。 */

/**
 * @brief 对一个「动态数组」进行排序。
 *
 * @note 该函数默认使用稳定的排序算法。并且混合使用「归并排序」与「快速排序」。
 * 如果在使用「归并排序」的过程中，出现内存不足的问题，则会回退到「快速排序」。
 *
 * @param darr 目标「动态数组」的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例
 * （两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值）。
 * @param ctx 比较函数的上下文参数。
 * @param desc 是否进行逆序排序。
 */
void darr_sort(
	darr_adt *darr,
	int (*cmp)(const void *, const void *, void *),
	void *ctx,
	bool desc
) ATTRS_NONNULL(1, 2);

/**
 * @brief 对一个「动态数组」进行反转。
 *
 * @param darr 目标「动态数组」的指针。
 */
void darr_reverse(
	darr_adt *darr
) ATTRS_NONNULL(1);

#endif /* DYNAMIC_ARRAY_H */
