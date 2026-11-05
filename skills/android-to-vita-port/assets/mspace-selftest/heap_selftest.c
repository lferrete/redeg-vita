/* Copyright (C) 2026 Leandro Ferrete. See LICENSE. */
#include "utils/heap_selftest.h"
#include "utils/logger.h"
#ifdef DEBUG_SOLOADER
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/processmgr.h>
#include <stdint.h>
#include <string.h>

enum { WORKERS = 4, SLOTS = 16, ROUNDS = 256, ARENA_SIZE = 1024 * 1024 };
typedef struct {
    unsigned char *pointer;
    unsigned size;
} HeapSlot;
/* Static so a timed-out worker cannot retain a pointer to a returned stack. */
static struct {
    SceClibMspace heap;
    uintptr_t base;
    uint64_t deadline;
    unsigned workers, arrivals, phase, abort, result, running;
    unsigned bad_worker, bad_round, bad_slot;
    unsigned cpu_mask;
    HeapSlot slots[WORKERS][SLOTS];
} s_probe;

static void failure(unsigned result, unsigned worker, unsigned round, unsigned slot) {
    unsigned expected = 0;
    if (__atomic_compare_exchange_n(&s_probe.result, &expected, result, 0,
                                    __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
        s_probe.bad_worker = worker; s_probe.bad_round = round; s_probe.bad_slot = slot;
    }
}
static int rendezvous(void) {
    unsigned phase = __atomic_load_n(&s_probe.phase, __ATOMIC_ACQUIRE);
    if (__atomic_add_fetch(&s_probe.arrivals, 1, __ATOMIC_ACQ_REL) == s_probe.workers) {
        __atomic_store_n(&s_probe.arrivals, 0, __ATOMIC_RELAXED);
        __atomic_store_n(&s_probe.phase, phase + 1, __ATOMIC_RELEASE);
        return !__atomic_load_n(&s_probe.abort, __ATOMIC_RELAXED);
    }
    while (__atomic_load_n(&s_probe.phase, __ATOMIC_ACQUIRE) == phase) {
        if (__atomic_load_n(&s_probe.abort, __ATOMIC_RELAXED)) return 0;
        if (sceKernelGetProcessTimeWide() > s_probe.deadline) {
            failure(5, 0, 0, 0);
            __atomic_store_n(&s_probe.abort, 1, __ATOMIC_RELAXED); return 0;
        }
        sceKernelDelayThread(50);
    }
    return !__atomic_load_n(&s_probe.abort, __ATOMIC_RELAXED);
}
static unsigned char pattern(unsigned worker, unsigned round, unsigned slot, unsigned byte) {
    return (unsigned char)(worker * 71 + round * 17 + slot * 37 + byte * 13);
}
static int heap_worker(SceSize length, void *argument) {
    unsigned worker = *(unsigned *)argument;
    for (unsigned round = 0; round < ROUNDS; ++round) {
        if (!rendezvous()) return 0;
        int cpu = sceKernelGetThreadCpuAffinityMask(sceKernelGetThreadId());
        if (cpu >= 0) __atomic_fetch_or(&s_probe.cpu_mask, (unsigned)cpu, __ATOMIC_RELAXED);
        for (unsigned slot = 0; slot < SLOTS; ++slot) {
            HeapSlot *entry = &s_probe.slots[worker][slot];
            unsigned bytes = 32 + ((worker * 379 + slot * 97 + round * 43) & 2047);
            entry->size = bytes;
            entry->pointer = sceClibMspaceMalloc(s_probe.heap, bytes);
            uintptr_t address = (uintptr_t)entry->pointer;
            if (address < s_probe.base || address > s_probe.base + ARENA_SIZE ||
                bytes > s_probe.base + ARENA_SIZE - address) {
                failure(1, worker, round, slot); continue;
            }
            for (unsigned byte = 0; byte < bytes; ++byte)
                entry->pointer[byte] = pattern(worker, round, slot, byte);
        }
        if (!rendezvous()) return 0;
        if (!worker && !__atomic_load_n(&s_probe.result, __ATOMIC_RELAXED)) {
            for (unsigned first = 0; first < s_probe.workers * SLOTS; ++first) {
                HeapSlot a = s_probe.slots[first / SLOTS][first % SLOTS];
                for (unsigned second = first + 1; second < s_probe.workers * SLOTS; ++second) {
                    HeapSlot b = s_probe.slots[second / SLOTS][second % SLOTS];
                    if ((uintptr_t)a.pointer < (uintptr_t)b.pointer + b.size &&
                        (uintptr_t)b.pointer < (uintptr_t)a.pointer + a.size)
                        failure(2, first / SLOTS, round, first % SLOTS);
                }
                if (!__atomic_load_n(&s_probe.result, __ATOMIC_RELAXED))
                    for (unsigned byte = 0; byte < a.size; ++byte)
                        if (a.pointer[byte] != pattern(first / SLOTS, round, first % SLOTS, byte)) {
                            failure(3, first / SLOTS, round, first % SLOTS); break;
                        }
            }
        }
        if (!rendezvous() || __atomic_load_n(&s_probe.result, __ATOMIC_RELAXED)) return 0;
        for (unsigned slot = 0; slot < SLOTS; ++slot) {
            sceClibMspaceFree(s_probe.heap, s_probe.slots[worker][slot].pointer);
            s_probe.slots[worker][slot].pointer = NULL;
        }
    }
    return 0;
}
static int run_mode(unsigned workers) {
    memset(&s_probe, 0, sizeof(s_probe));
    s_probe.running = 1; s_probe.workers = workers;
    SceUID block = sceKernelAllocMemBlock("tasm_private_heap_test", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, ARENA_SIZE, NULL);
    void *base = NULL;
    if (block < 0 || sceKernelGetMemBlockBase(block, &base) < 0) {
        if (block >= 0) sceKernelFreeMemBlock(block);
        s_probe.running = 0; return -1;
    }
    s_probe.base = (uintptr_t)base;
    s_probe.heap = sceClibMspaceCreate(base, ARENA_SIZE);
    if (!s_probe.heap) { sceKernelFreeMemBlock(block); s_probe.running = 0; return -2; }
    s_probe.deadline = sceKernelGetProcessTimeWide() + 6000000;
    SceUID threads[WORKERS]; unsigned started[WORKERS] = {0}, ids[WORKERS];
    for (unsigned i = 0; i < workers; ++i) {
        ids[i] = i;
        threads[i] = sceKernelCreateThread("tasm_heap_test", heap_worker, 0xA0, 0x10000, 0,
                                          0x10000u << (i % 3), NULL);
        if (threads[i] < 0) {
            l_error("[heap-selftest] CreateThread worker=%u ret=%x", i, threads[i]);
            failure(4, i, 0, 0); __atomic_store_n(&s_probe.abort, 1, __ATOMIC_RELAXED);
        }
    }
    uint64_t began = sceKernelGetProcessTimeWide();
    for (unsigned i = 0; i < workers; ++i)
        if (threads[i] >= 0) {
            int ret = sceKernelStartThread(threads[i], sizeof(ids[i]), &ids[i]);
            if (ret >= 0) started[i] = 1;
            else {
                l_error("[heap-selftest] StartThread worker=%u ret=%x", i, ret);
                failure(4, i, 0, 0); __atomic_store_n(&s_probe.abort, 1, __ATOMIC_RELAXED);
            }
        }
    int joined = 1;
    for (unsigned i = 0; i < workers; ++i) {
        if (threads[i] < 0) continue;
        SceUInt timeout = 8000000;
        if (started[i] && sceKernelWaitThreadEnd(threads[i], NULL, &timeout) < 0) {
            joined = 0; failure(6, i, 0, 0);
            __atomic_store_n(&s_probe.abort, 1, __ATOMIC_RELAXED);
        } else sceKernelDeleteThread(threads[i]);
    }
    l_info("[heap-selftest] workers=%u result=%u worker=%u round=%u slot=%u elapsed_us=%llu affinity_mask=%x joined=%d private_arena=1MiB",
        workers, s_probe.result, s_probe.bad_worker, s_probe.bad_round, s_probe.bad_slot,
        (unsigned long long)(sceKernelGetProcessTimeWide() - began), s_probe.cpu_mask, joined);
    if (!joined) {
        /* Retain static context and arena until process close, never free memory
         * that a timed-out worker could still touch. Do not run this test again. */
        l_error("[heap-selftest] worker timeout; close app before further testing"); return -3;
    }
    if (s_probe.result && s_probe.result != 4) {
        l_error("[heap-selftest] failed private arena retained; close app before further testing");
        return s_probe.result;
    }
    sceClibMspaceDestroy(s_probe.heap);
    sceKernelFreeMemBlock(block);
    s_probe.running = 0;
    return s_probe.result;
}
void tasm_heap_selftest(void) {
    if (s_probe.running) { l_warn("[heap-selftest] previous workers still active; close app"); return; }
    l_info("[heap-selftest] explicit diagnostic, exclude FPS; private Sony mspace, no game heap mutation");
    int serial = run_mode(1);
    int concurrent = serial ? -1 : run_mode(WORKERS);
    l_info("[heap-selftest] done serial=%d concurrent=%d; a pass is bounded stress coverage, not lifetime proof", serial, concurrent);
}
#else
void tasm_heap_selftest(void) {}
#endif
