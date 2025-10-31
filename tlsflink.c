#include "tlsf.h"

#define POOL_SIZE (8 * 1024 * 1024)

extern char my_mem_pool[POOL_SIZE];
tlsf_t tlsf_instance = NULL;

void __attribute__((noinline)) my_init() {
    tlsf_instance = tlsf_create_with_pool((void *)my_mem_pool, POOL_SIZE);
}

void * __attribute__((noinline)) my_malloc(size_t size) {
    if (size > 4096) {
        return NULL; // 拒绝
    }
    return tlsf_malloc(tlsf_instance, size);
}

void __attribute__((noinline)) my_free(void *ptr) {
    tlsf_free(tlsf_instance, ptr);
}