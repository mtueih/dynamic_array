#include "dynamic_array.h"

#include <assert.h>
#include <safe_calc.h>
#include <stdlib.h>
#include <string.h>

/* C23 标准引入了 nullptr 关键字，因此当在 C23 以下标准时将宏定义为 nullptr，否则定义为 NULL。 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define NULLPTR nullptr
#elif
#  define NULLPTR NULL
#endif

#define DARR_MAX(a, b) ((a) > (b) ? (a) : (b))
#define DARR_MIN(a, b) ((a) < (b) ? (a) : (b))

// 「动态字符串」抽象数据类型定义
struct dynamic_array {
	// 元素相关成员变量。
	size_t em_sz;
	// 数组相关成员变量。
	void *data;
	size_t len;
	// 容量相关成员变量。
	// 上面的数组长度「len」以及下面的两个容量相关的成员变量，都以元素个数为单位。
	size_t cap;
	size_t min_cap;
};


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define EM_MEMMOVE(dst, src, dst_pos, src_pos, em_size, len) memmove((char *)(dst) + (dst_pos) * (em_size), (char *)(src) + (src_pos) * (em_size), (len) * (em_size))
#define EM_MEMCPY(dst, src, dst_pos, src_pos, em_size, len) memcpy((char *)(dst) + (dst_pos) * (em_size), (char *)(src) + (src_pos) * (em_size), (len) * (em_size))
// #define ARG_CHECK(exp, v) if(exp) return v;
#define EMS_MOVE(darr, end, start) memove((char *)((darr)->data) + (end) * ((darr)->em_sz), (char *)((darr)->data) + (start) * ((darr)->em_sz), (((darr)->len) - (start)) * ((darr)->em_sz))

// 静态函数声明
// 动态调整容量
/**
 * 调整一个「动态数组」的容量。本质上就是 realloc 的封装。
 * 会执行乘法溢出判断。
 * @warning new_len 不能为 0，否则可能导致未定义行为。
 * @param darr 目标「动态数组」指针。
 * @param new_len 目标容量长度（以元素个数为单位而不是字节）。不能为 0。
 * @return 调整成功返回 true，失败返回 false。
 */
static bool capacity_resize(darr_adt *darr, size_t new_len);

/**
 * 常规调整一个「动态数组」的容量。
 * 与 capacity_resize 不同之处在于，参数 new_len 可为 0，届时会单独进行释放，避免 realloc 未定义行为。
 * @param darr 目标「动态数组」指针。
 * @param new_len 目标容量长度（以元素个数为单位而不是字节）。
 * @return 调整成功返回 true，失败返回 false。
 */
static bool capacity_resize_regular(darr_adt *darr, size_t new_len);

/**
 * 动态调整一个「动态数组」的容量。
 * 会进行预分配和延迟兼容，以摊销 realloc 函数调用开销。
 * @param darr 目标「动态数组」指针。
 * @param new_len 目标容量长度（以元素个数为单位而不是字节）。
 * @return 调整成功返回 true，失败返回 false。
 */
static bool capacity_resize_dynamic(darr_adt *darr, size_t new_len);

/**
 * 插入若干元素到一个「动态数组」中。
 * 将繁琐的，容易出错的 memmove 和 memcpy 操作封装成一个函数。
 * @param darr 目标「动态数组」指针。
 * @param index 目标位置索引。
 * @param elements 被插入元素起始指针。
 * @param count 插入元素个数。
 * @return 全局状态码。
 */
static int elements_insert(darr_adt *darr, size_t index, const void *elements, size_t count);

/**
 * 删除一个「动态数组」中的若干元素。
 * @param darr 目标「动态数组」指针。
 * @param index 被删除元素起始位置索引。
 * @param count 删除元素个数。
 */
static void elements_remove(darr_adt *darr, size_t index, size_t count);

static bool resize_cap(darr_adt *darr, size_t new_len);

// 实际调整容量
static bool resize_cap_actual(darr_adt *darr, size_t new_cap);

static size_t calculate_adjusted_cap(const darr_adt *darr, size_t needed_len);

// static bool resize_cap2(DArray *darr, size_t new_cap);

// 创建，销毁，清空。
darr_adt *darr_create(const size_t element_size, const size_t length) {
	darr_adt *new_darr;

	// 元素大小为 0，视为创建失败，或者创建空气。
	if (element_size == 0) return DARR_NULLPTR;

	// 为「动态数组」容器分配内存。
	new_darr = malloc(sizeof(darr_adt));
	if (new_darr == DARR_NULLPTR) return DARR_NULLPTR;

	// 初始化「动态数组」。
	new_darr->em_sz = element_size;
	new_darr->len = 0;

	// 为「动态数组」预分配容量。
	// 如果不用预分配，就初始化剩余成员变量后返回「动态数组」指针。
	if (length == 0) {
		new_darr->data = DARR_NULLPTR;
		new_darr->min_cap = new_darr->cap = 0;

		return new_darr;
	}

	// 调用静态函数 capacity_resize 执行分配。
	// capacity_resize 函数应该执行实际的容量调整操作，参数指定多大就分配多大。
	// 其本质上应该只是 realloc 函数的调用。
	// 如果调整成功，其应该对且仅对目标「动态数组」的 data 和 cap 两个成员变量做出改动。
	if (!capacity_resize(new_darr, length)) {
		// 如果分配失败，则视为「动态数组」创建失败，释放「动态数组」容器内存。
		free(new_darr);
		return DARR_NULLPTR;
	}

	// 如果分配成功，那么将成员变量「min_cap」设置为 length。（将预分配行为视为一种保底预期。）
	new_darr->min_cap = length;

	return new_darr;
}

void darr_destroy(darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 如果容量不为 0，则释放。
	if (darr->data != DARR_NULLPTR) free(darr->data);

	// 释放「动态数组」容器内存。
	free(darr);
}

void darr_clear(darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 清空「动态数组」，不会“立即”对内存以及数据做出改动。
	// 因此只将其「长度」置为 0 即可。
	darr->len = 0;
}


// 属性获取与设置，以及元素访问。
void *darr_carr(darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	return darr->data;
}

void *darr_at(darr_adt *const darr, const size_t index) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 检查索引是否越界。
	if (index >= darr->len) return DARR_NULLPTR;

	return (char *) darr->data + index * darr->em_sz;
}

size_t darr_element_size(const darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	return darr->em_sz;
}

size_t darr_length(const darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	return darr->len;
}

bool darr_is_empty(const darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	return darr->len == 0;
}

size_t darr_capacity(const darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	return darr->cap;
}

int darr_set_capacity(darr_adt *const darr, const size_t new_capacity) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 调用 capacity_resize_regular 函数，执行容量调整操作。
	// 该函数与 capacity_resize 函数不同之处在于，当目标容量为 0 时，
	// 其单独进行释放，避免 realloc 的未定义行为。
	if (!capacity_resize_regular(darr, new_capacity)) {
		// 如果调整失败，大概率是分配失败。（实际上如果计算溢出，那么内存分配必然会失败。）
		return DARR_MEMORY_ALLOC_FAILED;
	}

	// 如果 capacity_resize_regular 成功，那么修改成员变量「min_cap」的值。
	// 因为将 darr_set_capacity 函数调用行为，视为一种保底预期。
	darr->min_cap = new_capacity;

	// 如果新的容量小于先前的元素个数，那么原有内容会被截断。
	// 因此需要更新「length」的值。
	if (new_capacity < darr->len) {
		darr->len = new_capacity;
	}

	return DARR_SUCCESS;
}

int darr_reserve(darr_adt *const darr, const size_t new_capacity) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 此函数与「darr_set_capacity」不同之处在于，此函数不会截断当前内容。
	// 因此需要取「new_capacity」和「darr->len」的较大值来进行实际的调整。
	// 即，当「new_capacity」小于「darr->len」时，实际相当于执行 darr_shrink_to_fit。
	if (!capacity_resize_regular(darr, DARR_MAX(new_capacity, darr->len))) {
		// 如果调整失败。
		return DARR_MEMORY_ALLOC_FAILED;
	}

	// 如果 capacity_resize_regular 成功，那么修改成员变量「min_cap」的值。
	darr->min_cap = new_capacity;

	return DARR_SUCCESS;
}

void darr_shrink_to_fit(darr_adt *const darr) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 执行此函数，无论实际调整成功与否，都将「min_cap」的值置为 0。
	// 因为调用此函数的行为，视为放弃保底预期。
	darr->min_cap = 0;

	// 执行 capacity_resize_regular（在此函数中，成功与否并不重要，而且由于不是扩容，所以几乎不会失败）。
	capacity_resize_regular(darr, darr->len);
}


// 增减元素。
int darr_append(darr_adt *const darr, const void *const element) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);
	assert(element != DARR_NULLPTR);

	// 由于所有增减元素函数执行操作都大体不差，并且考虑到可维护性，
	// 因此将操作统一抽象封装为一个函数，通过调用该函数来完成。

	// 在数组末尾追加一个元素，等价于在末尾（下标为「darr->len」的位置）插入一个元素。
	return elements_insert(darr, darr->len, element, 1);
}

int darr_append_n(darr_adt *const darr, const void *const elements, const size_t count) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);
	assert(elements != DARR_NULLPTR);

	// 参数检查。
	// 检查 count 是否为 0，如果为 0，视为什么都不追加，那么操作是一定成功的。
	if (count == 0) return DARR_SUCCESS;

	// 在数组末尾追加多个元素，等价于在末尾（下标为「darr->len」的位置）插入多个元素。
	return elements_insert(darr, darr->len, elements, count);
}

int darr_prepend(darr_adt *const darr, const void *const element) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);
	assert(element != DARR_NULLPTR);

	// 在数组开头追加一个元素，等价于在开头（下标为「0」的位置）插入一个元素。
	return elements_insert(darr, 0, element, 1);
}

int darr_prepend_n(darr_adt *const darr, const void *const elements, const size_t count) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);
	assert(elements != DARR_NULLPTR);

	// 参数检查。
	// 检查 count 是否为 0，如果为 0，视为什么都不追加，那么操作是一定成功的。
	if (count == 0) return DARR_SUCCESS;

	// 在数组开头追加多个元素，等价于在开头（下标为「0」的位置）插入多个元素。
	return elements_insert(darr, 0, elements, count);
}

int darr_insert(darr_adt *const darr, const size_t index, const void *const element) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);
	assert(element != DARR_NULLPTR);

	// 参数检查。
	// 检查下标是否越界。（对于插入操作，索引等于「darr->len」是可以的，相当于尾部追加。）
	if (index > darr->len) return DARR_INVALID_PARAM;

	// 插入一个元素。
	return elements_insert(darr, index, element, 1);
}

int darr_insert_n(darr_adt *const darr, const size_t index, const void *const elements, const size_t count) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);
	assert(elements != DARR_NULLPTR);

	// 参数检查。
	// 检查下标是否越界。（对于插入操作，索引等于「darr->len」是可以的，相当于尾部追加。）
	if (index > darr->len) return DARR_INVALID_PARAM;
	// 检查 count 是否为 0，如果为 0，视为什么都不插入，那么操作是一定成功的。
	if (count == 0) return DARR_SUCCESS;

	// 插入多个元素。
	return elements_insert(darr, index, elements, count);
}

void darr_remove(darr_adt *const darr, const size_t index) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 参数检查。
	// 检查下标是否越界。
	if (index >= darr->len) return;

	elements_remove(darr, index, 1);
}

void darr_remove_n(darr_adt *const darr, const size_t index, const size_t count) {
	// 开发阶段参数检查。
	assert(darr != DARR_NULLPTR);

	// 参数检查。
	// 检查下标是否越界。
	if (index >= darr->len) return;
	// 检查 index + count 是否超过数组末尾。
	if (!safe_size_add_test(index, count) || index + count > darr->len) return;

	elements_remove(darr, index, count);
}

// ADT 操作。
darr_adt *darr_clone(const darr_adt *const darr) {

}

// 遍历
void darr_foreach(darr_adt *darr, void (*func)(void *)) {
	char *p;

	if (!darr || !darr->len || !func)
		return;

	for (p = (char *) darr->data; p < (char *) darr->data + darr->len; ++p) {
		func(p);
	}
}

void darr_foreach_const(const darr_adt *darr, void (*func)(const void *)) {
	darr_foreach(darr, func);
}

// 查询
bool darr_contains(const darr_adt *darr, const void *element, int (*cmp)(const void *, const void *)) {
	return darr_find_const(darr, element, cmp, false) != NULL;
}

void *darr_find(darr_adt *darr, const void *element, int (*cmp)(const void *, const void *), bool backward) {
	char *p;

	if (!darr || !darr->len || !element || !cmp)
		return NULL;

	for (
		p = backward
			    ? (char *) darr->data + (darr->len - 1) * darr->em_sz
			    : (char *) darr->data;
		backward
			? p >= (char *) darr->data
			: p < (char *) darr->data + darr->len * darr->em_sz;
		backward
			? (p -= darr->em_sz)
			: (p += darr->em_sz)) {
		if (cmp(p, element) == 0) {
			return p;
		}
	}
	return NULL;
}

const void *darr_find_const(
	const darr_adt *darr, const void *element, int (*cmp)(const void *, const void *), bool backward
) {
	return darr_find(darr, element, cmp, backward);
}

// 排序
void darr_sort(darr_adt *darr, int (*cmp)(const void *, const void *), bool desc) {
	char *temp; // 临时分配内存
	size_t width; // 归并排序中，子数组宽度
	char *j, *k, *l; // 用于在循环中迭代
	size_t i; // 用于在循环中迭代
	char *mid, *right; // 归并排序中，子数组的 3 个位置
	bool src_is_data; // 归并排序中，用于乒乓策略
	char *src, *dst; // 归并排序中，用于乒乓策略

	// 参数检查
	if (!darr || darr->len < 2 || !cmp)
		return;
	// assert(p_darr != NULL && cmp != NULL);
	// if (p_darr->m_length < 2) return;
	// 优先考虑归并排序，如果内存分配失败回退到选择排序
	{
		// 分配临时数组用于归并
		temp = malloc(darr->len * darr->em_sz);
		if (temp == NULL)
			goto selection_sort;

		src_is_data = true;
		// 从长度为1的子数组开始，逐步倍增
		for (width = 1; width < darr->len; width *= 2) {
			src = src_is_data ? (char *) darr->data : temp;
			dst = src_is_data ? temp : (char *) darr->data;
			// 归并相邻的两个有序子数组
			for (i = 0; i < darr->len; i += 2 * width) {
				mid = src + MIN(i + width, darr->len) * darr->em_sz;
				right = src + MIN(i + 2 * width, darr->len) * darr->em_sz;

				j = src + i * darr->em_sz;
				k = mid;
				l = dst + i * darr->em_sz;
				// 归并 arr[left:mid] 和 arr[mid:right] 到 temp
				while (j < mid && k < right) {
					if (desc ? cmp(j, k) > 0 : cmp(j, k) < 0) {
						memcpy(l, j, darr->em_sz);
						j += darr->em_sz;
					} else {
						memcpy(l, k, darr->em_sz);
						k += darr->em_sz;
					}
					l += darr->em_sz;
				}
				while (j < mid) {
					memcpy(l, j, darr->em_sz);
					j += darr->em_sz;
					l += darr->em_sz;
				}
				while (k < right) {
					memcpy(l, k, darr->em_sz);
					k += darr->em_sz;
					l += darr->em_sz;
				}
			}
			src_is_data = !src_is_data;
		}
		if (!src_is_data) {
			// 将临时数组内容复制回原数组
			memcpy(darr->data, temp, darr->len * darr->em_sz);
		}
		free(temp);
	}
selection_sort: {
		temp = malloc(darr->em_sz);
		if (temp == NULL)
			return;

		for (j = (char *) darr->data;
		     j < (char *) darr->data + (darr->len - 1) * darr->em_sz;
		     j += darr->em_sz) {
			for (k = j, l = j;
			     k < (char *) darr->data + darr->len * darr->em_sz;
			     k += darr->em_sz) {
				if (desc ? cmp(k, l) > 0 : cmp(k, l) < 0) {
					l = k;
				}
			}
			if (l != j) {
				memcpy(temp, j, darr->em_sz);
				// ELE_CPY(temp, j);
				memcpy(k, l, darr->em_sz);
				// ELE_CPY(k, l);
				memcpy(l, temp, darr->em_sz);
				// ELE_CPY(l, temp);
			}
		}
		free(temp);
	}
}

// ADT 操作
void darr_swap(darr_adt *darr_1, darr_adt *darr_2) {
	darr_adt temp;

	if (!darr_1 || !darr_2 || darr_1->em_sz != darr_2->em_sz)
		return;

	temp = *darr_1;
	*darr_1 = *darr_2;
	*darr_2 = temp;
}

/******************************************************************************
************************       静态函数实现       *******************************
******************************************************************************/
static bool capacity_resize(darr_adt *const darr, const size_t new_len) {
	void *new_data;
	size_t new_size;

	// 安全执行 size_t 乘法（new_len * darr->em_sz），防止溢出。
	if (!safe_size_mul(new_len, darr->em_sz, &new_size)) {
		// 如果发生溢出，则视为调整失败。
		return false;
	}

	// 执行 realloc 操作。
	new_data = realloc(darr->data, new_size);
	// realloc 失败。
	if (new_data == DARR_NULLPTR) return false;

	// realloc 成功。
	darr->data = new_data;
	darr->cap = new_len;

	return true;
}

static bool capacity_resize_regular(darr_adt *const darr, const size_t new_len) {
	// 如果 new_len 为 0，单独进行释放，避免 realloc 的未定义行为。
	if (new_len == 0) {
		if (darr->data != DARR_NULLPTR) {
			free(darr->data);

			darr->data = DARR_NULLPTR;
			darr->cap = 0;
		}
		return true;
	}

	// 调用 capacity_resize 函数完成调整操作。
	return capacity_resize(darr, new_len);
}

static bool capacity_resize_dynamic(darr_adt *const darr, const size_t new_len) {
	size_t needed_len; // MAX(new_len, darr->min_cap)
	size_t adjusted_len;

	// 维持容量不低于保底值，因此取两者较大者。
	needed_len = DARR_MAX(new_len, darr->min_cap);

	// 如果「needed_len」等于当前容量，则不必进行任何改动，直接返回 true。
	if (needed_len == darr->cap) return true;

	// 如果「needed_len」小于当前容量，那么就延迟减容。
	if (needed_len < darr->cap) {
		// 仅当「needed_len」小于等于当前容量的一半时，才实际减容。
		// 即，当「needed_len」大于当前容量的一半时，直接返回 true。
		if (needed_len > darr->cap << 1) {
			return true;
		}
		// 否则执行容量调整。
		// 此处需要调用 capacity_resize_regular 而不是 capacity_resize，因为「needed_len」可能为 0。
		return capacity_resize_regular(darr, needed_len);
	}

	// 如果「needed_len」大于当前容量，那么就预分配容量（1.5 倍）。

	// 先尝试预分配容量，如果预分配容量调整失败，再回退到「needed_len」本身。
	// 安全执行 size_t 加法（needed_len + needed_len << 1），防止溢出。
	if (safe_size_add(needed_len, needed_len << 1, &adjusted_len)) {
		// 如果加法计算没有溢出，那就执行调整。
		// 此处调用 capacity_resize 而不是 capacity_resize_regular，因为如果是扩容，容量不可能是 0。
		if (capacity_resize(darr, adjusted_len)) {
			// 如果调整成功则返回 true。
			return true;
		}
	}

	// 如果加法出现溢出，或预分配容量调整失败，则回退到「needed_len」本身。
	return capacity_resize(darr, needed_len);
}

static int elements_insert(darr_adt *const darr, const size_t index, const void *const elements, const size_t count) {
	size_t new_len; // darr->len + count

	// 安全执行 size_t 加法（darr->len + count），防止溢出。
	if (!safe_size_add(darr->len, count, &new_len)) {
		// 溢出。
		return DARR_OVERFLOW;
	}

	// 调用 capacity_resize_dynamic 函数进行“动态”容量调整。
	// 该函数应该根据需求的容量进行动态调整，比如预分配一点内存，延迟兼容等，从而摊销 realloc 开销。
	// 并且该函数会维持容量不低于保底值「darr->min_cap」。
	if (!capacity_resize_dynamic(darr, new_len)) {
		return DARR_MEMORY_ALLOC_FAILED;
	}

	// 如果插入位置索引小于「darr->len」，则从插入位置起到数组末尾的现有元素都要进行移动。
	if (index < darr->len) {
		// 如果前面加法（darr->len + count）没有溢出，且容量调整成功，那么后续运算一定不会溢出。
		memmove(
			(char *) darr->data + (index + count) * darr->em_sz,
			(char *) darr->data + index * darr->em_sz,
			(darr->len - index) * darr->em_sz
		);
	}

	// 执行 memcpy。
	memcpy(
		(char *) darr->data + index * darr->em_sz,
		elements,
		count * darr->em_sz
	);

	// 更新数组长度。
	darr->len += count;

	return DARR_SUCCESS;
}

static void elements_remove(darr_adt *const darr, const size_t index, const size_t count) {
	// 如果被删除的元素不在末尾，就需要移动后面的元素。
	if (count && index + count < darr->len) {
		memmove(
			(char *) darr->data + index * darr->em_sz,
			(char *) darr->data + (index + count) * darr->em_sz,
			(darr->len - index - count) * darr->em_sz
		);
	}

	// 更新数组长度。
	darr->len -= count ? count : darr->len - index;

	// 动态减容。
	capacity_resize_dynamic(darr, darr->len);
}


static size_t calculate_adjusted_cap(const darr_adt *darr, size_t needed_len) {
	size_t new_cap, adj_cap;

	new_cap = needed_len * darr->em_sz;
	new_cap = MIN(new_cap, darr->min_cap);

	// 容量需求增加，动态向上扩容
	if (new_cap > darr->cap) {
		// 为需求容量的 1.5 倍
		adj_cap = new_cap + new_cap >> 1;
	}
	// 延迟减容
	// 当新的所需容量达到当前容量的 1/4 时，才实际减容
	else if (new_cap < darr->cap >> 2) {
		// 只减一半
		adj_cap = new_cap << 1;
	} else {
		// 无需减容
		return darr->cap;
	}

	adj_cap = adj_cap % sizeof(void *) == 0
			  ? adj_cap
			  : (adj_cap / sizeof(void *) + 1) * sizeof(void *);

	return adj_cap;
}

static bool resize_cap_actual(darr_adt *darr, size_t new_cap) {
	void *new_data;

	if (new_cap == darr->cap)
		return true;

	if (!new_cap) {
		if (darr->data) {
			free(darr->data);
			darr->data = NULL;
			darr->cap = 0;
		}
		return true;
	}

	if (!(new_data = realloc(darr->data, new_cap))) {
		return false;
	}

	darr->data = new_data;
	darr->cap = new_cap;
	return true;
}


darr_adt *darr_create_from_carr(size_t elem_size, size_t length, const void *carr, size_t count) {
	void *new_data;
	darr_adt *new_darr;

	if (!elem_size || !carr || !length || length < count)
		return NULL;

	new_data = NULL;

	if (!(new_data = malloc(length * elem_size)))
		return NULL;

	if (!(new_darr = malloc(sizeof(darr_adt)))) {
		free(new_data);
		return NULL;
	}

	memcpy(new_data, carr, count * elem_size);

	*new_darr = (darr_adt){.em_sz = elem_size};
	if (new_data) {
		new_darr->data = new_data;
		new_darr->len = count;
		new_darr->min_cap = new_darr->cap = length;
	}

	return new_darr;
}

darr_adt *darr_clone(const darr_adt *darr) {
	void *new_data;
	darr_adt *new_darr;

	if (!darr || !darr->em_sz)
		return NULL;

	new_data = NULL;

	if (darr->cap) {
		if (!(new_data = malloc(darr->cap)))
			return NULL;
	}

	if (!(new_darr = malloc(sizeof(darr_adt)))) {
		if (new_data)
			free(new_data);
		return NULL;
	}

	if (new_data)
		memcpy(new_data, darr->data, darr->cap);

	*new_darr = *darr;
	if (new_data) {
		new_darr->data = new_data;
	}

	return new_darr;
}

int darr_assign_carr(darr_adt *darr, const void *carr, size_t count) {
	void *new_data;

	if (!darr || !darr->em_sz || !carr || !count)
		return DARR_ARG_INVAL;

	if (darr->cap < count * darr->em_sz) {
		if (!resize_cap(darr, count)) {
			return DARR_MEM_LOC_FAILD;
		}
	}

	EM_MEMCPY(darr->data, carr, 0, 0, darr->em_sz, count);
	// memcpy(darr->data, carr, count * darr->em_sz);
	darr->len = count;
	return DARR_SUCCESS;
}

int darr_assign(darr_adt *dst, const darr_adt *src) {
	void *new_data;

	if (!dst || !dst->em_sz || !src || dst->em_sz != src->em_sz)
		return DARR_ARG_INVAL;

	if (dst->cap < src->len * dst->em_sz) {
		if (!resize_cap(dst, src->len)) {
			return DARR_MEM_LOC_FAILD;
		}
	}

	EM_MEMCPY(dst->data, src->data, 0, 0, dst->em_sz, src->len);
	// memcpy(dst->data, src->data, src->len * dst->em_sz);
	dst->len = src->len;
	return DARR_SUCCESS;
}

static bool resize_cap(darr_adt *darr, size_t new_len) {
	if (!resize_cap_actual(darr, calculate_adjusted_cap(darr, new_len))
	    && !resize_cap_actual(darr, new_len * darr->em_sz)
	)
		return false;
	return true;
}
