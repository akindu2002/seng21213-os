#ifndef PMM_H
#define PMM_H

#include "../include/types.h"

#define PMM_PAGE_SIZE 4096
#define PMM_MAX_MEMORY (32 * 1024 * 1024)
#define PMM_MAX_FRAMES (PMM_MAX_MEMORY / PMM_PAGE_SIZE)
#define PMM_BITMAP_SIZE (PMM_MAX_FRAMES / 8)

#define E820_MAP_ADDR 0x9000
#define E820_COUNT_ADDR 0x8FF0

#define E820_USABLE 1

typedef struct __attribute__((packed)) {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi;
} e820_entry_t;

void pmm_init(void);

uint32_t pmm_alloc_frame(void);
void pmm_free_frame(uint32_t paddr);

uint32_t pmm_get_total_frames(void);
uint32_t pmm_get_used_frames(void);
uint32_t pmm_get_free_frames(void);

#endif
