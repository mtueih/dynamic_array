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

#include "dynamic_array.h"

#include <safe_calc.h>
#include <stdlib.h>
#include <string.h>

/* C23 标准引入了 nullptr 关键字，因此当在 C23 以下标准时将宏定义为 nullptr，否则定义为 NULL。 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#  define DARR_NULLPTR nullptr
#else
#  define DARR_NULLPTR NULL
#endif

/* 排序算法阈值。当数组元素大于此值时使用适用于较大数据量的「归并排序」算法，
 * 否则使用适用于较小数据量的「插入排序」算法。 */
#define DARR_SORT_THRESHOLD 16

#define DARR_MAX(a, b) ((a) > (b) ? (a) : (b))
#define DARR_MIN(a, b) ((a) < (b) ? (a) : (b))

/* 某个「动态数组」的某个位置处的元素的指针表达式宏。 */
#define DARR_EM_AT(darr, i) ((char *) (darr)->data + (i) * (darr)->em_sz)


/* 「动态数组」抽象数据类型定义。 */
struct dynamic_array {
	/* 元素相关成员变量。 */
	size_t em_sz; /* 元素大小。单个元素占用的空间大小，以字节为单位。 */

	/* 数组相关成员变量。 */
	void *data; /* 指向数据缓冲区的指针。 */
	size_t len; /* 数组长度。元素个数。 */

	/* 容量相关成员变量。 */
	/* 上面的数组长度「len」以及下面的两个容量相关的成员变量，都以元素个数为单位。 */
	size_t cap;     /* 数组容量。能存放的元素个数，以元素个数为单位。 */
	size_t min_cap; /* 数组保底容量。在容量自动变化过程中维持不低于此值。 */
};


/***************************************************************
 *********************    静态函数声明。    **********************
 **************************************************************/

/* 容量调整。 */

/**
 * @brief 调整一个「动态数组」的容量。
 *
 * @note 本质上就是 realloc 的封装。会执行乘法溢出判断。
 *
 * @param darr 目标「动态数组」指针。
 * @param new_len 目标容量长度（以元素个数为单位而不是字节）。不能为 0。
 *
 * @return 调整成功返回 true，失败返回 false。
 */
static bool capacity_resize(
	darr_adt *darr,
	size_t new_len
);

/**
 * @brief 常规调整一个「动态数组」的容量。
 *
 * @note 与 capacity_resize 不同之处在于，参数 new_len 可为 0，届时会单独进行释放，避免 realloc 未定义行为。
 *
 * @param darr 目标「动态数组」指针。
 * @param new_len 目标容量长度（以元素个数为单位而不是字节）。
 *
 * @return 调整成功返回 true，失败返回 false。
 */
static bool capacity_resize_regular(
	darr_adt *darr,
	size_t new_len
);

/**
 * @brief 动态调整一个「动态数组」的容量。
 *
 * @note 会进行预分配和延迟减容，以摊销 realloc 函数调用开销。
 *
 * @param darr 目标「动态数组」指针。
 * @param new_len 目标容量长度（以元素个数为单位而不是字节）。
 *
 * @return 调整成功返回 true，失败返回 false。
 */
static bool capacity_resize_dynamic(
	darr_adt *darr,
	size_t new_len
);

/* 元素操作。 */

/**
 * @brief 插入若干元素到一个「动态数组」中。
 *
 * @note 将繁琐的，容易出错的 memmove 和 memcpy 操作封装成一个函数。
 *
 * @param darr 目标「动态数组」指针。
 * @param index 目标位置索引。
 * @param elements 被插入元素起始指针。
 * @param count 插入元素个数。
 *
 * @return 全局状态码。
 */
static darr_status_t elements_insert(
	darr_adt *darr,
	size_t index,
	const void *elements,
	size_t count
);

/**
 * @brief 删除一个「动态数组」中的若干元素。
 *
 * @param darr 目标「动态数组」指针。
 * @param index 被删除元素起始位置索引。
 * @param count 删除元素个数。
 */
static void elements_remove(
	darr_adt *darr,
	size_t index,
	size_t count
);

/* 排序算法实现。 */

/**
 * @brief 对一个「动态数组」进行插入排序。
 *
 * @param darr 目标「动态数组」的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例
 * （两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值）。
 * @param ctx 比较函数的上下文参数。
 * @param desc 是否进行逆序排序。
 *
 * @return 全局状态码。
 */
static darr_status_t insertion_sort(
	darr_adt *darr,
	int (*cmp)(const void *, const void *, void *),
	void *ctx,
	bool desc
);

/**
 * @brief 对一个「动态数组」进行归并排序。
 *
 * @param darr 目标「动态数组」的指针。
 * @param cmp 元素比较函数指针，其返回值因遵循 C 标准函数惯例
 * （两者相等返回 0，前者大于后者返回正值，前者小于后者返回负值）。
 * @param ctx 比较函数的上下文参数。
 * @param desc 是否进行逆序排序。
 *
 * @return 全局状态码。
 */
static darr_status_t merge_sort(
	darr_adt *darr,
	int (*cmp)(const void *, const void *, void *),
	void *ctx,
	bool desc
);


/***************************************************************
 *********************    API 函数定义。    *********************
 **************************************************************/

/* 创建，销毁，清空。 */

darr_adt *darr_create(
	const size_t element_size,
	const size_t length
) {
	darr_adt *new_darr;

	/* 元素大小为 0，视为创建失败，或者创建空气。 */
	if (element_size == 0) {
		return DARR_NULLPTR;
	}

	/* 为「动态数组」容器分配内存。 */
	new_darr = malloc(sizeof(darr_adt));
	if (new_darr == DARR_NULLPTR) {
		return DARR_NULLPTR;
	}

	/* 初始化「动态数组」。 */
	new_darr->em_sz = element_size;
	new_darr->data = DARR_NULLPTR;
	new_darr->len = 0;

	/**
	 * 为「动态数组」预分配容量。
	 * 如果不用预分配，就初始化剩余成员变量后返回「动态数组」指针。
	 */
	if (length == 0) {
		new_darr->min_cap = new_darr->cap = 0;

		return new_darr;
	}

	/**
	 * 调用静态函数 capacity_resize 执行分配。
	 * capacity_resize 函数应该执行实际的容量调整操作，参数指定多大就分配多大。
	 * 其本质上应该只是 realloc 函数的调用。
	 * 如果调整成功，其应该对且仅对目标「动态数组」的 data 和 cap 两个成员变量做出改动。
	 */
	if (!capacity_resize(new_darr, length)) {
		/* 如果分配失败，则视为「动态数组」创建失败，释放「动态数组」容器内存。 */
		free(new_darr);
		return DARR_NULLPTR;
	}

	/* 如果分配成功，那么将成员变量「min_cap」设置为 length。（将预分配行为视为一种保底预期。） */
	new_darr->min_cap = length;

	return new_darr;
}

void darr_destroy(
	darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return;
	}

	/* 如果「darr->data」不为空指针，则释放。 */
	if (darr->data != DARR_NULLPTR) {
		free(darr->data);
	}

	/* 释放「动态数组」容器内存。 */
	free(darr);
}

void darr_clear(
	darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return;
	}

	/**
	 * 清空「动态数组」，不会“立即”对内存以及数据做出改动。
	 * 因此只将其「长度」置为 0 即可。
	 */
	darr->len = 0;
}

/* 属性获取与设置，以及元素访问。 */

void *darr_carr(
	darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_NULLPTR;
	}

	return darr->data;
}

const void *darr_carr_const(
	const darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_NULLPTR;
	}

	return darr->data;
}

void *darr_at(
	darr_adt *const darr,
	const size_t index
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_NULLPTR;
	}

	/* 检查索引是否越界。 */
	if (index >= darr->len) {
		return DARR_NULLPTR;
	}

	return DARR_EM_AT(darr, index);
}

const void *darr_at_const(
	const darr_adt *const darr,
	const size_t index
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_NULLPTR;
	}

	/* 检查索引是否越界。 */
	if (index >= darr->len) {
		return DARR_NULLPTR;
	}

	return DARR_EM_AT(darr, index);
}

size_t darr_element_size(
	const darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return 0;
	}

	return darr->em_sz;
}

size_t darr_length(
	const darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return 0;
	}

	return darr->len;
}

bool darr_is_empty(
	const darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return true;
	}

	return darr->len == 0;
}

size_t darr_capacity(
	const darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return 0;
	}

	return darr->cap;
}

darr_status_t darr_set_capacity(
	darr_adt *const darr,
	const size_t new_capacity
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 如果 new_capacity 等于当前容量的话，什么也不用做，直接返回成功。 */
	if (new_capacity == darr->cap) {
		return DARR_SUCCESS;
	}

	/**
	 * 调用 capacity_resize_regular 函数，执行容量调整操作。
	 * 该函数与 capacity_resize 函数不同之处在于，当目标容量为 0 时，
	 * 其单独进行释放，避免 realloc 的未定义行为。
	 */
	if (!capacity_resize_regular(darr, new_capacity)) {
		/* 如果调整失败，大概率是分配失败。（实际上如果计算溢出，那么内存分配必然会失败。） */
		return DARR_MEMORY_ALLOC_FAILED;
	}

	/**
	 * 如果 capacity_resize_regular 成功，那么修改成员变量「min_cap」的值。
	 * 因为将 darr_set_capacity 函数调用行为，视为一种保底预期。
	 */
	darr->min_cap = new_capacity;

	/**
	 * 如果新的容量小于先前的元素个数，那么原有内容会被截断。
	 * 因此需要更新「length」的值。
	 */
	if (new_capacity < darr->len) {
		darr->len = new_capacity;
	}

	return DARR_SUCCESS;
}

darr_status_t darr_reserve(
	darr_adt *const darr,
	const size_t new_capacity
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 如果 new_capacity 等于当前容量的话，什么也不用做，直接返回成功。 */
	if (new_capacity == darr->cap) {
		return DARR_SUCCESS;
	}

	/* 如果 new_capacity 小于当前数组长度的话，那么视为无效参数，因为此函数不能截断现有内容。 */
	/* 实际上，如果 new_capacity 小于当前数组长度，应当返回成功，因为此函数并没有约束参数，只是从行为上确保不截断现有内容。 */
	if (new_capacity < darr->len) {
		return DARR_SUCCESS;
	}

	/**
	 * 此函数与「darr_set_capacity」不同之处在于，此函数不会截断当前内容。
	 * 因此需要取「new_capacity」和「darr->len」的较大值来进行实际的调整。
	 * 即，当「new_capacity」小于「darr->len」时，实际相当于执行 darr_shrink_to_fit。
	 */
	if (!capacity_resize_regular(darr, new_capacity)) {
		/* 如果调整失败。 */
		return DARR_MEMORY_ALLOC_FAILED;
	}

	/* 如果 capacity_resize_regular 成功，那么修改成员变量「min_cap」的值。 */
	darr->min_cap = new_capacity;

	return DARR_SUCCESS;
}

void darr_shrink_to_fit(
	darr_adt *const darr
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return;
	}

	/**
	 * 执行此函数，无论实际调整成功与否，都将「min_cap」的值置为 0。
	 * 因为调用此函数的行为，视为放弃保底预期。
	 */
	if (darr->min_cap != 0) {
		darr->min_cap = 0;
	}

	/* 如果当前容量已经等于数组长度了，那就直接返回。 */
	if (darr->cap == darr->len) {
		return;
	}

	/**
	 * 执行 capacity_resize_regular
	 * （在此函数中，成功与否并不重要，而且由于不是扩容，所以几乎不会失败）。
	 */
	capacity_resize_regular(darr, darr->len);
}

/* 增减元素。 */

darr_status_t darr_append(
	darr_adt *const darr,
	const void *const element
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR || element == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/**
	 * 由于所有增减元素函数执行操作都大体不差，并且考虑到可维护性，
	 * 因此将操作统一抽象封装为一个函数，通过调用该函数来完成。
	 */
	/* 在数组末尾追加一个元素，等价于在末尾（下标为「darr->len」的位置）插入一个元素。 */
	return elements_insert(darr, darr->len, element, 1);
}

darr_status_t darr_append_n(
	darr_adt *const darr,
	const void *const elements,
	const size_t count
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR || elements == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 检查 count 是否为 0，如果为 0，视为什么都不追加，那么操作是一定成功的。 */
	if (count == 0) {
		return DARR_SUCCESS;
	}

	/* 在数组末尾追加多个元素，等价于在末尾（下标为「darr->len」的位置）插入多个元素。 */
	return elements_insert(darr, darr->len, elements, count);
}

darr_status_t darr_prepend(
	darr_adt *const darr,
	const void *const element
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR || element == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 在数组开头追加一个元素，等价于在开头（下标为「0」的位置）插入一个元素。 */
	return elements_insert(darr, 0, element, 1);
}

darr_status_t darr_prepend_n(
	darr_adt *const darr,
	const void *const elements,
	const size_t count
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR || elements == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 检查 count 是否为 0，如果为 0，视为什么都不追加，那么操作是一定成功的。 */
	if (count == 0) {
		return DARR_SUCCESS;
	}

	/* 在数组开头追加多个元素，等价于在开头（下标为「0」的位置）插入多个元素。 */
	return elements_insert(darr, 0, elements, count);
}

darr_status_t darr_insert(
	darr_adt *const darr,
	const size_t index,
	const void *const element
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR || element == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 检查下标是否越界。（对于插入操作，索引等于「darr->len」是可以的，相当于尾部追加。） */
	if (index > darr->len) {
		return DARR_INVALID_PARAM;
	}

	/* 插入一个元素。 */
	return elements_insert(darr, index, element, 1);
}

darr_status_t darr_insert_n(
	darr_adt *const darr,
	const size_t index,
	const void *const elements,
	const size_t count
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR || elements == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 检查下标是否越界。（对于插入操作，索引等于「darr->len」是可以的，相当于尾部追加。） */
	if (index > darr->len) {
		return DARR_INVALID_PARAM;
	}
	/* 检查 count 是否为 0，如果为 0，视为什么都不插入，那么操作是一定成功的。 */
	if (count == 0) {
		return DARR_SUCCESS;
	}

	/* 插入多个元素。 */
	return elements_insert(darr, index, elements, count);
}

darr_status_t darr_remove(
	darr_adt *const darr,
	const size_t index
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 检查下标是否越界。 */
	if (index >= darr->len) {
		return DARR_INVALID_PARAM;
	}

	elements_remove(darr, index, 1);

	return DARR_SUCCESS;
}

darr_status_t darr_remove_n(
	darr_adt *const darr,
	const size_t index,
	const size_t count
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 检查下标是否越界。 */
	if (index >= darr->len) {
		return DARR_INVALID_PARAM;
	}
	/* 检查 index + count 是否超过数组末尾。 */
	if (!safe_size_t_add(index, count, DARR_NULLPTR) || index + count > darr->len) {
		return DARR_INVALID_PARAM;
	}

	elements_remove(darr, index, count);

	return DARR_SUCCESS;
}

/* 元素操作。 */

darr_status_t darr_swap(
	darr_adt *const darr,
	const size_t index_1,
	const size_t index_2
) {
	void *temp;
	void *elem_1, *elem_2;
	bool hasFreeCap;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 检查下标是否越界。 */
	/* 如果两下标相等，也直接返回。 */
	if (index_1 == index_2 || index_1 >= darr->len || index_2 >= darr->len) {
		return DARR_INVALID_PARAM;
	}

	/* 如果当前数组的容量大于长度，那么直接使用空闲容量来存储临时数据。 */
	hasFreeCap = darr->cap > darr->len;
	if (hasFreeCap) {
		temp = DARR_EM_AT(darr, darr->cap - 1);
	} else {
		temp = malloc(darr->em_sz);

		/* 如果分配失败，直接返回。 */
		if (temp == DARR_NULLPTR) {
			return DARR_MEMORY_ALLOC_FAILED;
		}
	}

	elem_1 = DARR_EM_AT(darr, index_1);
	elem_2 = DARR_EM_AT(darr, index_2);

	/* 备份第一个元素的数据。 */
	memcpy(temp, elem_1, darr->em_sz);

	/* 第二个元素复制到第一个元素。 */
	memcpy(elem_1, elem_2, darr->em_sz);

	/* 备份的第一个元素复制到第二个元素。 */
	memcpy(elem_2, temp, darr->em_sz);

	/* 释放临时分配的内存。 */
	if (!hasFreeCap) {
		free(temp);
	}

	return DARR_SUCCESS;
}

/* ADT 操作。 */

darr_adt *darr_clone(
	const darr_adt *const darr
) {
	darr_adt *new_darr;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_NULLPTR;
	}

	/* 为新的「动态数组」容器分配内存。 */
	new_darr = malloc(sizeof(darr_adt));
	if (new_darr == DARR_NULLPTR) return DARR_NULLPTR;

	/* 初始化新数组。 */
	new_darr->em_sz = darr->em_sz;
	new_darr->data = DARR_NULLPTR;

	/* 如果源数组是个空数组，且容量为 0，则直接初始化剩余成员变量后返回。 */
	if (darr->data == DARR_NULLPTR) {
		new_darr->min_cap = new_darr->cap = 0;
		new_darr->len = 0;

		return new_darr;
	}

	/* 为新「动态数组」的缓冲区分配内存。 */
	if (!capacity_resize(new_darr, darr->cap)) {
		free(new_darr);
		return DARR_NULLPTR;
	}

	/* 如果缓冲区内存分配成功，则更新新数组「min_cap」。 */
	new_darr->min_cap = darr->min_cap;

	/* 拷贝原数组缓冲区有效数据。 */
	memcpy(new_darr->data, darr->data, darr->len * darr->em_sz);

	/* 更新数组长度。 */
	new_darr->len = darr->len;

	return new_darr;
}

/* 遍历。 */

void darr_foreach(
	darr_adt *const darr,
	void (*const func)(void *, void *),
	void *const ctx,
	const bool backward
) {
	char *p;
	char *end, *start;
	size_t darr_em_sz;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR || func == DARR_NULLPTR) {
		return;
	}

	/* 参数检查。 */
	/* 如果数组为空，则直接返回。 */
	if (darr->len == 0) return;

	start = DARR_EM_AT(darr, 0);
	end = DARR_EM_AT(darr, darr->len);
	darr_em_sz = darr->em_sz;

	if (backward) {
		for (p = end; p > start;) {
			p -= darr_em_sz;

			func(p, ctx);
		}
	} else {
		for (p = start; p < end; p += darr_em_sz) {
			func(p, ctx);
		}
	}
}

void darr_foreach_const(
	const darr_adt *const darr,
	void (*const func)(const void *, void *),
	void *const ctx,
	const bool backward
) {
	const char *p;
	const char *end, *start;
	size_t darr_em_sz;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR || func == DARR_NULLPTR) {
		return;
	}

	/* 参数检查。 */
	/* 如果数组为空，则直接返回。 */
	if (darr->len == 0) return;

	start = DARR_EM_AT(darr, 0);
	end = DARR_EM_AT(darr, darr->len);
	darr_em_sz = darr->em_sz;

	if (backward) {
		for (p = end; p > start;) {
			p -= darr_em_sz;

			func(p, ctx);
		}
	} else {
		for (p = start; p < end; p += darr_em_sz) {
			func(p, ctx);
		}
	}
}

/* 查询、查找。 */

bool darr_contains(
	const darr_adt *const darr,
	bool (*const predicate)(const void *, void *),
	void *const ctx,
	const bool backward
) {
	const char *p;
	const char *end, *start;
	size_t darr_em_sz;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR || predicate == DARR_NULLPTR) {
		return false;
	}

	/* 参数检查。 */
	/* 如果数组为空，则一定不包含。 */
	if (darr->len == 0) return false;

	start = DARR_EM_AT(darr, 0);
	end = DARR_EM_AT(darr, darr->len);
	darr_em_sz = darr->em_sz;

	if (backward) {
		for (p = end; p > start;) {
			p -= darr_em_sz;

			if (predicate(p, ctx)) {
				return true;
			}
		}
	} else {
		for (p = start; p < end; p += darr_em_sz) {
			if (predicate(p, ctx)) {
				return true;
			}
		}
	}

	return false;
}

bool darr_find(
	const darr_adt *const darr,
	bool (*const predicate)(const void *, void *),
	void *const ctx,
	size_t *const out_index,
	const bool backward
) {
	const char *p;
	const char *end, *start;
	size_t darr_em_sz;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR || predicate == DARR_NULLPTR) {
		return false;
	}

	/* 参数检查。 */
	/* 如果数组为空，则一定不包含。 */
	if (darr->len == 0) return false;

	start = DARR_EM_AT(darr, 0);
	end = DARR_EM_AT(darr, darr->len);
	darr_em_sz = darr->em_sz;

	if (backward) {
		for (p = end; p > start;) {
			p -= darr_em_sz;

			if (predicate(p, ctx)) {
				goto finded;
			}
		}
	} else {
		for (p = start; p < end; p += darr_em_sz) {
			if (predicate(p, ctx)) {
				goto finded;
			}
		}
	}

	return false;

finded:
	if (out_index != DARR_NULLPTR) {
		*out_index = (p - start) / darr_em_sz;
	}

	return true;
}

bool darr_find_n(
	const darr_adt *const darr,
	bool (*const predicate)(const void *, void *),
	void *const ctx,
	const size_t n,
	size_t *const out_index,
	const bool backward
) {
	const char *p;
	const char *end, *start;
	size_t find_count;
	size_t darr_em_sz;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR || predicate == DARR_NULLPTR) {
		return false;
	}

	/* 参数检查。 */
	/* 如果数组为空，则一定不包含。 */
	if (darr->len == 0) return false;

	find_count = 0;
	start = DARR_EM_AT(darr, 0);
	end = DARR_EM_AT(darr, darr->len);
	darr_em_sz = darr->em_sz;

	if (backward) {
		for (p = end; p > start;) {
			p -= darr_em_sz;

			if (predicate(p, ctx)) {
				++find_count;

				if (find_count == n) {
					goto finded;
				}
			}
		}
	} else {
		for (p = start; p < end; p += darr_em_sz) {
			if (predicate(p, ctx)) {
				++find_count;

				if (find_count == n) {
					goto finded;
				}
			}
		}
	}

	/* n 为 0 表示最后一次。 */
	if (n == 0 && find_count != 0) {
		goto finded;
	}

	return false;

finded:
	if (out_index != DARR_NULLPTR) {
		*out_index = (p - start) / darr_em_sz;
	}

	return true;
}

size_t darr_count(
	const darr_adt *const darr,
	bool (*const predicate)(const void *, void *),
	void *const ctx
) {
	const char *p;
	const char *end, *start;
	size_t count;
	size_t darr_em_sz;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR || predicate == DARR_NULLPTR) {
		return 0;
	}

	/* 参数检查。 */
	/* 如果数组为空，则一定不包含。 */
	if (darr->len == 0) {
		return 0;
	}

	count = 0;
	start = DARR_EM_AT(darr, 0);
	end = DARR_EM_AT(darr, darr->len);
	darr_em_sz = darr->em_sz;

	for (p = start; p < end; p += darr_em_sz) {
		if (predicate(p, ctx)) {
			++count;
		}
	}

	return count;
}

bool darr_find_binary(
	const darr_adt *const darr,
	const void *const element,
	int (*const cmp)(const void *, const void *, void *),
	void *const ctx,
	size_t *const out_index,
	const bool desc
) {
	size_t mid, left, right;
	int ret;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR || element == DARR_NULLPTR || cmp == DARR_NULLPTR) {
		return false;
	}

	/* 参数检查。 */
	/* 如果数组为空，则一定不包含。 */
	if (darr->len == 0) {
		return false;
	}

	left = 0;
	right = darr->len;

	while (left < right) {
		/* 防止 (left + right) 导致溢出。 */
		mid = (left & right) + ((left ^ right) >> 1);

		/* 处理逆序排序的情况。 */
		if (desc) {
			ret = cmp(element, DARR_EM_AT(darr, mid), ctx);
		} else {
			ret = cmp(DARR_EM_AT(darr, mid), element, ctx);
		}

		if (ret == 0) {
			goto finded;
		}

		if (ret > 0) {
			right = mid;
		} else {
			left = mid + 1;
		}
	}

	return false;

finded:
	if (out_index != DARR_NULLPTR) {
		*out_index = mid;
	}

	return true;
}

/* 排序、反转。 */

darr_status_t darr_sort(
	darr_adt *const darr,
	int (*const cmp)(const void *, const void *, void *),
	void *const ctx,
	const bool desc
) {
	/* 参数检查。 */
	if (darr == DARR_NULLPTR || cmp == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查 */
	/* 如果数组元素不足 2 个，那么就无需进行排序。 */
	if (darr->len < 2) {
		return DARR_SUCCESS;
	}

	/* 当数组元素个数大于「排序算法阈值」时，使用「归并排序」，否则使用「插入排序」。 */
	if (darr->len > DARR_SORT_THRESHOLD) {
		if (merge_sort(darr, cmp, ctx, desc) == DARR_SUCCESS) {
			return DARR_SUCCESS;
		}
	}

	/* 否则。或者「归并排序」失败，回退到「插入排序」。 */
	return insertion_sort(darr, cmp, ctx, desc);
}

darr_status_t darr_reverse(
	darr_adt *const darr
) {
	char *p, *q;
	char *tmp;
	bool has_free_cap;

	/* 参数检查。 */
	if (darr == DARR_NULLPTR) {
		return DARR_INVALID_PARAM;
	}

	/* 参数检查。 */
	/* 如果「动态数组」元素个数小于 2，那么无需反转，直接返回。 */
	if (darr->len < 2) {
		return DARR_SUCCESS;
	}

	/* 如果「动态数组」容量大于长度，则直接使用空闲容量来存储临时数据。 */
	has_free_cap = (darr->cap > darr->len);
	if (has_free_cap) {
		tmp = DARR_EM_AT(darr, darr->cap - 1);
	} else {
		/* 否则分配内存来存储。 */
		tmp = malloc(darr->em_sz);
		/* 如果分配失败则直接返回。 */
		if (tmp == DARR_NULLPTR) {
			return DARR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 初始化迭代变量。 */
	p = DARR_EM_AT(darr, 0);
	q = DARR_EM_AT(darr, darr->len - 1);

	while (p < q) {
		/* 交换两元素。 */
		memcpy(tmp, p, darr->em_sz);
		memcpy(p, q, darr->em_sz);
		memcpy(q, tmp, darr->em_sz);

		/* 迭代。 */
		p += darr->em_sz;
		q -= darr->em_sz;
	}

	/* 释放临时分配的内存。 */
	if (!has_free_cap) {
		free(tmp);
	}

	return DARR_SUCCESS;
}


/***************************************************************
 *********************    静态函数实现。    **********************
 **************************************************************/

/* 容量调整。 */

static bool capacity_resize(
	darr_adt *const darr,
	const size_t new_len
) {
	void *new_data;
	size_t new_size;

	/* 安全执行 size_t 乘法（new_len * darr->em_sz），防止溢出。 */
	if (!safe_size_t_mul(new_len, darr->em_sz, &new_size)) {
		/* 如果发生溢出，则视为调整失败。 */
		return false;
	}

	/* 执行 realloc 操作。 */
	new_data = realloc(darr->data, new_size);
	/* realloc 失败。 */
	if (new_data == DARR_NULLPTR) return false;

	/* realloc 成功。 */
	darr->data = new_data;
	darr->cap = new_len;

	return true;
}

static bool capacity_resize_regular(
	darr_adt *const darr,
	const size_t new_len
) {
	/* 如果 new_len 为 0，单独进行释放，避免 realloc 的未定义行为。 */
	if (new_len == 0) {
		if (darr->data != DARR_NULLPTR) {
			free(darr->data);

			darr->data = DARR_NULLPTR;
			darr->cap = 0;
		}
		return true;
	}

	/* 调用 capacity_resize 函数完成调整操作。 */
	return capacity_resize(darr, new_len);
}

static bool capacity_resize_dynamic(
	darr_adt *const darr,
	const size_t new_len
) {
	size_t needed_len; /* MAX(new_len, darr->min_cap) */
	size_t adjusted_len;

	/* 维持容量不低于保底值，因此取两者较大者。 */
	needed_len = DARR_MAX(new_len, darr->min_cap);

	/* 如果「needed_len」等于当前容量，则不必进行任何改动，直接返回 true。 */
	if (needed_len == darr->cap) return true;

	/* 如果「needed_len」小于当前容量，那么就延迟减容。 */
	if (needed_len < darr->cap) {
		/**
		 * 仅当「needed_len」小于等于当前容量的一半时，才实际减容。
		 * 即，当「needed_len」大于当前容量的一半时，直接返回 true。
		 */
		if (needed_len > (darr->cap >> 1)) {
			return true;
		}

		/**
		 * 否则执行容量调整。
		 * 此处需要调用 capacity_resize_regular 而不是 capacity_resize，因为「needed_len」可能为 0。
		 */
		return capacity_resize_regular(darr, needed_len);
	}

	/* 如果「needed_len」大于当前容量，那么就预分配容量（1.5 倍）。 */

	/* 先尝试预分配容量，如果预分配容量调整失败，再回退到「needed_len」本身。 */
	/* 安全执行 size_t 加法（needed_len + needed_len >> 1），防止溢出。 */
	if (safe_size_t_add(needed_len, needed_len >> 1, &adjusted_len)) {
		/**
		 * 如果加法计算没有溢出，那就执行调整。
		 * 此处调用 capacity_resize 而不是 capacity_resize_regular，因为如果是扩容，「adjusted_len」不可能是 0。
		 */
		if (capacity_resize(darr, adjusted_len)) {
			/* 如果调整成功则返回 true。 */
			return true;
		}
	}

	/* 如果加法出现溢出，或预分配容量调整失败，则回退到「needed_len」本身。 */
	return capacity_resize(darr, needed_len);
}

/* 元素操作。 */

static darr_status_t elements_insert(
	darr_adt *const darr,
	const size_t index,
	const void *const elements,
	const size_t count
) {
	size_t new_len; /* darr->len + count */

	/* 安全执行 size_t 加法（darr->len + count），防止溢出。 */
	if (!safe_size_t_add(darr->len, count, &new_len)) {
		/* 溢出。 */
		return DARR_MEMORY_ALLOC_FAILED;
	}

	/**
	 * 调用 capacity_resize_dynamic 函数进行“动态”容量调整。
	 * 该函数应该根据需求的容量进行动态调整，比如预分配一点内存，延迟减容等，从而摊销 realloc 开销。
	 * 并且该函数会维持容量不低于保底值「darr->min_cap」。
	 */
	if (!capacity_resize_dynamic(darr, new_len)) {
		return DARR_MEMORY_ALLOC_FAILED;
	}

	/* 如果插入位置索引小于「darr->len」，则从插入位置起到数组末尾的现有元素都要进行移动。 */
	if (index < darr->len) {
		/* 如果前面加法（darr->len + count）没有溢出，且容量调整成功，那么后续运算一定不会溢出。 */
		/**
		 * index 在上层检查是否越界，前面又成功执行（darr->len + count），
		 * 而 index 如果不越界， 它一定小于等于 darr->len，所以 index + count 一定不会溢出。
		 * 同时 (darr->len - index) 也不会下溢。
		 * 而 （index + count）* darr->em_sz，由底层容量调整函数 capacity_resize 进行溢出检查，所以不会溢出。
		 * 如果其他部分的逻辑都是严谨的，那么任何 [0, darr->len] 区间的整数 * darr->em_sz 都一定不会溢出。
		 */
		memmove(
			DARR_EM_AT(darr, index + count),
			DARR_EM_AT(darr, index),
			(darr->len - index) * darr->em_sz
		);
	}

	/* 执行 memcpy。 */
	memcpy(
		DARR_EM_AT(darr, index),
		elements,
		count * darr->em_sz
	);

	/* 更新数组长度。 */
	darr->len += count;

	return DARR_SUCCESS;
}

static void elements_remove(
	darr_adt *const darr,
	const size_t index,
	const size_t count
) {
	/* 如果被删除的元素不在末尾，就需要移动后面的元素。 */
	if (count != 0 && index + count < darr->len) {
		memmove(
			DARR_EM_AT(darr, index),
			DARR_EM_AT(darr, index + count),
			(darr->len - index - count) * darr->em_sz
		);
	}

	/* 更新数组长度。 */
	darr->len -= (count != 0) ? (count) : (darr->len - index);

	/* 动态减容。 */
	capacity_resize_dynamic(darr, darr->len);
}

/* 排序算法实现。 */

static darr_status_t insertion_sort(
	darr_adt *const darr,
	int (*const cmp)(const void *, const void *, void *),
	void *const ctx,
	const bool desc
) {
	size_t i, j;
	int cmp_ret;
	char *tmp;
	bool has_free_cap;

	/* 如果当前数组有空闲容量，则使用它来存储临时数据，否则就新分配内存。 */
	has_free_cap = (darr->cap > darr->len);
	if (has_free_cap) {
		tmp = DARR_EM_AT(darr, darr->cap - 1);
	} else {
		tmp = malloc(darr->em_sz);

		if (tmp == DARR_NULLPTR) {
			return DARR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 遍历每个「未排序元素」。 */
	for (i = 1; i < darr->len; ++i) {
		char *key = DARR_EM_AT(darr, i);

		/* 在「已排序区域」中寻找正确位置。 */
		for (j = i; j > 0; --j) {
			cmp_ret = cmp(key, DARR_EM_AT(darr, j - 1), ctx);
			if ((!desc && cmp_ret >= 0) ||
			    (desc && cmp_ret <= 0)
			) {
				break;
			}
		}
		/* 插入到正确位置（如果需要）。 */
		if (j < i) {
			/* 先拷贝待插入元素。 */
			memcpy(tmp, key, darr->em_sz);

			/* 向后移动「已排序区域」中，目标插入位置向后的元素。 */
			memmove(DARR_EM_AT(darr, j + 1), DARR_EM_AT(darr, j), (i - j) * darr->em_sz);

			/* 拷贝待插入元素到正确位置。 */
			memcpy(DARR_EM_AT(darr, j), tmp, darr->em_sz);
		}
	}

	if (!has_free_cap) free(tmp);

	return DARR_SUCCESS;
}

static darr_status_t merge_sort(
	darr_adt *const darr,
	int (*const cmp)(const void *, const void *, void *),
	void *const ctx,
	const bool desc
) {
	char *tmp;         /* 指向临时内存的指针。 */
	bool has_free_cap; /* 源数组是否有足够空闲容量。 */

	size_t darr_len, darr_cap, darr_em_sz; /* 用于缓存成员变量。 */
	size_t darr_size;                      /* 缓存 darr_len * darr_em_sz。 */
	char *darr_data;                       /* 用于缓存成员变量。 */

	/**
	 * 用于「乒乓策略」，当用于存放归并结果的缓冲区被写满一次（执行完一轮归并后）后，
	 * 此时，「归并结果缓冲区」是可以完整拷贝写入一次「数组缓冲区」之后再进行下一轮归并的。
	 * 但是这样存在性能浪费。「乒乓策略」则是每进行一轮归并后，交替两缓冲区的身份，
	 * 即，将「归并结果缓冲区」视为「数组缓冲区」，将「数组缓冲区」视为「归并结果缓冲区」。
	 * 最后，如果缓冲区身份是错位的，那就进行一次拷贝即可。
	 * 这样可以大幅减少大数据快的拷贝次数。
	 */
	bool src_is_data; /* 指向数组缓冲区的指针是 src 还是 dst。 */
	char *src, *dst;  /* 归并排序「乒乓策略」中，指向「数组缓冲区」的指针，和指向用于存放归并结果的缓冲区的指针。 */

	size_t width; /* 归并排序中，「子数组」的宽度。用于迭代。 */
	size_t i;     /* 归并排序中，相邻两「子数组」对的起始下标。用于迭代。 */
	/**
	 * 归并排序中，用于标识相邻两「子数组」对中，
	 * 第 1 个「子数组」的结束位置（同时也是第 2 个「子数组」的起始位置），
	 * 和第 2 个「子数组」的结束位置的指针。
	 * 用于迭代过程中的边界指针。
	 */
	char *mid, *right;
	/**
	 * 归并排序中，j、k 分别指向两「子数组」中下一个待比较元素。
	 * l 指向「归并结果缓冲区」中下一个可写入位置。
	 */
	char *j, *k, *l;

	int cmp_ret; /* 存储元素比较函数的返回值。 */

	/**
	 * 分配临时内存用于归并。
	 * 如果目前数组空闲容量大于等于数组长度的两倍，那么直接使用数组空闲容量作为临时内存。
	 * 即，数组长度小于等于容量的一半。
	 */
	darr_len = darr->len;
	darr_cap = darr->cap;
	darr_em_sz = darr->em_sz;
	darr_size = darr_len * darr_em_sz;

	has_free_cap = (darr_len <= (darr_cap >> 1));

	if (has_free_cap) {
		tmp = DARR_EM_AT(darr, darr_cap - darr_len);
	} else {
		tmp = malloc(darr_size);

		if (tmp == DARR_NULLPTR) {
			return DARR_MEMORY_ALLOC_FAILED;
		}
	}

	/* 最开始，数组数据确实位于「数组缓冲区」中。 */
	src_is_data = true;
	darr_data = darr->data;

	/**
	 * 从宽度为 1 的「子数组」开始，每一轮都处理当前宽度下，每两个相邻「子数组」的归并。
	 * 每一轮归并后，意味着每个宽度为 width * 2 的「子数组」都已经是有序的，
	 * 下一轮则是归并每两个相邻的，宽度为 width * 2 的「子数组」。
	 * 因此每一轮结束后，width 都 * 2。
	 * 一个数组最多能分成两个「子数组」，因此，循环的最后一轮，width 应该是数组长度 / 2。
	 * 之后循环就应该停止。最后一轮之后，width * 2 后，必然 >= 数组长度。
	 * 因此将 width < darr_len 作为循环条件。
	 */
	for (width = 1; width < darr_len; width <<= 1) {
		/**
		 * 「乒乓策略」，每轮结束后，交替两缓冲区的身份。
		 * src：指向「数组缓冲区」，即归并所处理的数据所在的缓冲区。
		 * dst：指向「归并结果缓冲区」，即存放归并结果所使用的缓冲区。
		 */
		src = src_is_data ? darr_data : tmp;
		dst = src_is_data ? tmp : darr_data;

		/**
		 * 归并当前一轮的「子数组」宽度下，每两个相邻的「子数组」，结果写入「归并结果缓冲区」。
		 * 每次迭代处理两个「子数组」，当然也有一个「子数组」的情况，当「子数组」的个数是奇数时。
		 * 那么迭代可以通过一个指向缓冲区某位置的指针来进行，每轮迭代后，指针都要跨越两个「子数组」，
		 * 最开始，指向整个「数组缓冲区」的第一个「子数组」。
		 * 当指针指向「数组缓冲区」之外时，停止循环。
		 */

		for (i = 0; i < darr_len; i += (width << 1)) {
			/**
			 * 每轮当前迭代中，我们要归并 p 指向位置向后的两个相邻「子数组」。
			 * 为此，我们需要分别知道两个「子数组」的起始位置和结束位置（使用前闭后开区间）。
			 * 第一个「子数组」的起始位置是 p，毫无疑问。
			 * 第二个「子数组」的起始位置，同时也是第一个「子数组」的结束位置，【我们用一个 mid 指针表示。】
			 * 应该是 p 向后移动 1 个「子数组」的宽度，即 p + width * darr_em_sz。
			 * 而第二个「子数组」的结束位置，【我们用一个 right 指针表示。】
			 * 应该是 p 向后移动 2 个「子数组」的宽度，即 mid + width * darr_em_sz。
			 *
			 * 但情况并不总是这样，当总「子数组」个数为奇数时，最后一轮拿到的，只有一个「子数组」，
			 * 即 p 后面只有一个「子数组」，如何处理第二个「子数组」不存在的情况？
			 * 我们在处理第二个「子数组」的起始位置和结束位置的时候，
			 * 第二个「子数组」的起始位置正常为 p 向后移动 1 个「子数组」的宽度即可。
			 * 而第二个「子数组」的结束位置，应该与其起始位置是重叠的。
			 * 如何做到这一点？我们只需要确保第二个「子数组」的结束位置的指针不超过数组缓冲区的结束位置即可。
			 */
			mid = src + DARR_MIN(i + width, darr_len) * darr_em_sz;
			right = src + DARR_MIN(i + (width << 1), darr_len) * darr_em_sz;

			/**
			 * 归并过程中，需要 3 个指针变量。
			 * j、k 分别指向两个「子数组」中需要比较的元素；
			 * l 指向「归并结果缓冲区」中下一个可写入位置。
			 */
			j = src + i * darr_em_sz;
			k = mid;

			l = dst + i * darr_em_sz;

			/**
			 * 同时依次遍历两个「子数组」。每两个元素一对相比较，符合目标顺序大小的，先放入「归并结果缓冲区」。
			 * 如果其中一个「子数组」已经遍历完，那么下面这个循环就不会再执行。
			 * 这也兼顾了只有一个「子数组」的情况，此时此循环压根不会执行。
			 * 后续直接将没遍历完的那个「子数组」的剩余元素原封不动写入「归并结果缓冲区」。
			 */
			while (j < mid && k < right) {
				cmp_ret = cmp(j, k, ctx);

				/**
				 * 在顺序排序的情况下，如果 j <= k（ret <= 0），则优先写入 j，否则写入 k；
				 * 在逆序排序的情况下，如果 j >= k（ret >= 0），则优先写入 j，否则写入 k。
				 */
				if ((!desc && cmp_ret <= 0) ||
				    (desc && cmp_ret >= 0)
				) {
					/* 写入 j 后，j 指向其「子数组」中的下一个元素。 */
					memcpy(l, j, darr_em_sz);
					j += darr_em_sz;
				} else {
					/* 写入 k 后，k 指向其「子数组」中的下一个元素。 */
					memcpy(l, k, darr_em_sz);
					k += darr_em_sz;
				}

				/* l 被写入后，指向「归并结果缓冲区」的下一个位置。 */
				l += darr_em_sz;
			}

			/* 没遍历完的「子数组」剩余元素原封不动写入「归并结果缓冲区」。 */
			/* 如果是第 1 个「子数组」没遍历完。 */
			if (j < mid) {
				memcpy(l, j, mid - j);
			}
			/* 如果是第 2 个「子数组」没遍历完。 */
			else if (k < mid) {
				memcpy(l, k, right - k);
			}
		}

		/* 「乒乓策略」：交替两缓冲区的身份。 */
		src_is_data = !src_is_data;
	}

	/* 「乒乓策略」：如果两缓冲区身份错位，则将数据写回「数组缓冲区」。 */
	if (!src_is_data) {
		memcpy(darr_data, tmp, darr_size);
	}

	/* 释放临时分配的内存。 */
	if (!has_free_cap) {
		free(tmp);
	}

	return DARR_SUCCESS;
}
