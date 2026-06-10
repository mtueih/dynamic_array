#include <stdio.h>

#include "dynamic_array.h"

void func(const void *el);

int main(void) {
    printf("Hello, World!\n");
    darr_foreach(darr_create(0, 0), func);
    return 0;
}
