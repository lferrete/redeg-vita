#ifndef REDEG_MACHO_H
#define REDEG_MACHO_H

#include <stdint.h>

typedef struct {
    uint32_t load_base;   /* address of the Mach-O header page (vmaddr 0x1000) */
    uint32_t delta;       /* load_base - 0x1000 */
    uint32_t image_size;
    uint32_t entry;       /* _main */
    uint32_t text_addr, text_size;
    uint32_t classlist;
    int nclasses;
    uint32_t lc_symbol_stub;
    uint32_t lc_symbol_stub_size;
} macho_image_t;

extern macho_image_t g_macho;

int macho_load(const char *path, macho_image_t *out);
void macho_flush(void);
void macho_add_patch(uint32_t vmaddr, uint32_t word);
void macho_add_branch(uint32_t vmaddr, uint32_t target);
const char *macho_symbol_name(uint32_t addr);

#endif
