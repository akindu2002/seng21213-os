#include "pmm.h"

/*
 * Stage 3 - Physical Memory Manager
 *
 * One bitmap bit represents one 4 KB physical page frame.
 *
 * The BIOS E820 memory map is provided by boot/stage2.asm
 * at physical address 0x9000.
 */

static uint8_t frame_bitmap[PMM_BITMAP_SIZE];

static uint32_t total_frames = 0;
static uint32_t used_frames = 0;

static void bitmap_set(uint32_t frame)
{
    frame_bitmap[frame / 8] |=
        (uint8_t)(1u << (frame % 8));
}

static void bitmap_clear(uint32_t frame)
{
    frame_bitmap[frame / 8] &=
        (uint8_t)~(1u << (frame % 8));
}

static bool bitmap_test(uint32_t frame)
{
    return (frame_bitmap[frame / 8] &
            (uint8_t)(1u << (frame % 8))) != 0;
}

static void mark_region_free(uint64_t base, uint64_t length)
{
    uint64_t end;
    uint32_t first_frame;
    uint32_t last_frame;
    uint32_t frame;

    if (length == 0) {
        return;
    }

    end = base + length;

    if (base >= PMM_MAX_MEMORY) {
        return;
    }

    if (end > PMM_MAX_MEMORY) {
        end = PMM_MAX_MEMORY;
    }

    first_frame = (uint32_t)((base + PMM_PAGE_SIZE - 1) /
                             PMM_PAGE_SIZE);

    last_frame = (uint32_t)(end / PMM_PAGE_SIZE);

    if (first_frame >= last_frame) {
        return;
    }

    if (last_frame > PMM_MAX_FRAMES) {
        last_frame = PMM_MAX_FRAMES;
    }

    for (frame = first_frame; frame < last_frame; frame++) {
        if (bitmap_test(frame)) {
            bitmap_clear(frame);

            if (used_frames > 0) {
                used_frames--;
            }
        }
    }
}

void pmm_init(void)
{
    e820_entry_t *map;
    uint16_t count;
    uint16_t i;

    /*
     * Start with every frame marked as used.
     */
    for (i = 0; i < PMM_BITMAP_SIZE; i++) {
        frame_bitmap[i] = 0xFF;
    }

    total_frames = PMM_MAX_FRAMES;
    used_frames = total_frames;

    /*
     * Read the E820 map written by the stage-2 bootloader.
     */
    map = (e820_entry_t *)E820_MAP_ADDR;
    count = *(uint16_t *)E820_COUNT_ADDR;

    /*
     * Mark E820 type-1 regions as free.
     */
    for (i = 0; i < count; i++) {
        if (map[i].type == E820_USABLE) {
            mark_region_free(map[i].base, map[i].length);
        }
    }

    /*
     * Keep the first 1 MB reserved.
     */
    for (i = 0; i < (1024 * 1024) / PMM_PAGE_SIZE; i++) {
        if (!bitmap_test(i)) {
            bitmap_set(i);
            used_frames++;
        }
    }
}

uint32_t pmm_alloc_frame(void)
{
    uint32_t frame;

    for (frame = 0; frame < total_frames; frame++) {
        if (!bitmap_test(frame)) {
            bitmap_set(frame);
            used_frames++;

            return frame * PMM_PAGE_SIZE;
        }
    }

    return 0;
}

void pmm_free_frame(uint32_t paddr)
{
    uint32_t frame;

    if (paddr == 0 || paddr >= PMM_MAX_MEMORY) {
        return;
    }

    if ((paddr % PMM_PAGE_SIZE) != 0) {
        return;
    }

    frame = paddr / PMM_PAGE_SIZE;

    if (bitmap_test(frame)) {
        bitmap_clear(frame);

        if (used_frames > 0) {
            used_frames--;
        }
    }
}

uint32_t pmm_get_total_frames(void)
{
    return total_frames;
}

uint32_t pmm_get_used_frames(void)
{
    return used_frames;
}

uint32_t pmm_get_free_frames(void)
{
    return total_frames - used_frames;
}
