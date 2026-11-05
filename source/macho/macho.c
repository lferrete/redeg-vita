#include "macho/macho.h"
#include "utils/logger.h"

#include <psp2/kernel/sysmem.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/processmgr.h>
#include <kubridge.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

macho_image_t g_macho;

/* ---- minimal Mach-O definitions ---- */
#define MH_MAGIC 0xfeedface
#define LC_SEGMENT 0x1
#define LC_SYMTAB 0x2
#define LC_DYSYMTAB 0xb
#define LC_LOAD_DYLIB 0xc

typedef struct {
    uint32_t cmd, cmdsize;
} load_command_t;

typedef struct {
    uint32_t cmd, cmdsize;
    char segname[16];
    uint32_t vmaddr, vmsize, fileoff, filesize;
    uint32_t maxprot, initprot, nsects, flags;
} segment_command_t;

typedef struct {
    char sectname[16], segname[16];
    uint32_t addr, size, offset, align, reloff, nreloc, flags, reserved1, reserved2;
} section_t;

typedef struct {
    uint32_t cmd, cmdsize;
    uint32_t symoff, nsyms, stroff, strsize;
} symtab_command_t;

typedef struct {
    uint32_t cmd, cmdsize;
    uint32_t ilocalsym, nlocalsym, iextdefsym, nextdefsym;
    uint32_t iundefsym, nundefsym, tocoff, ntoc, modtaboff, nmodtab;
    uint32_t extrefsymoff, nextrefsyms, indirectsymoff, nindirectsyms;
    uint32_t extreloff, nextrel, locreloff, nlocrel;
} dysymtab_command_t;

typedef struct {
    int32_t r_address;
    uint32_t r_info;
} relocation_info_t;

typedef struct {
    uint32_t n_strx;
    uint8_t n_type, n_sect;
    int16_t n_desc;
    uint32_t n_value;
} nlist_t;

typedef struct {
    char segname[16];
    uint32_t vmaddr, vmsize, fileoff, filesize, nsects;
    section_t *sects;
} seg_t;

static uint8_t *s_file;
static size_t s_filesize;
static symtab_command_t *s_symtab;
static dysymtab_command_t *s_dysym;
static nlist_t *s_syms;
static const char *s_strtab;
static seg_t s_segs[8];
static int s_nsegs;
static uint32_t s_max_vm;
static uint32_t *s_sym_addrs = NULL;
static int s_nsym_addrs = 0;

static int cmp_u32(const void *a, const void *b) {
    uint32_t x = *(const uint32_t *)a, y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}
static int sym_has(uint32_t v) {
    int lo = 0, hi = s_nsym_addrs - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (s_sym_addrs[mid] == v) return 1;
        if (s_sym_addrs[mid] < v) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}
static section_t *find_section(const char *seg, const char *name);
static int in_section_range(const char *seg, const char *name, uint32_t v) {
    section_t *s = find_section(seg, name);
    return s && v >= s->addr && v < s->addr + s->size;
}
static int looks_like_function(uint32_t v) {
    uint32_t a = v & ~1u;
    /* All functions outside _g_s3e_code have symbols in LC_SYMTAB and are
     * matched by sym_has(). Only stripped game code in _g_s3e_code
     * (0x35600..0x112be8 and veneers 0x131254..0x134738) needs inspection. */
    if (!((a >= 0x35600 && a < 0x112be8) || (a >= 0x131254 && a < 0x134738)))
        return 0;
    if (v & 1) return 1;
    if ((a & 3) != 0) return 0;
    uint32_t off = a - 0x1000;
    uint32_t w = *(uint32_t *)(s_file + off);
    if ((w & 0xFFFF0000) == 0xE92D0000) return 1;  /* push */
    if ((w & 0xFFFF0FFF) == 0xE52D0004) return 1;  /* str rX, [sp, #-4]! */
    if (w == 0xE1A0C00D) return 1;                 /* mov ip, sp */
    if ((w & 0xFFFFF000) == 0xE24DD000) return 1;  /* sub sp, #imm */
    if (w == 0xE12FFF1E || w == 0xE1A0F00E) return 1; /* bx lr / mov pc,lr */
    if ((w & 0x0FFFFFF0) == 0x012FFF10) return 1;  /* bx rX veneer */
    if ((w & 0xFF000000) == 0xEA000000) return 1;  /* b target */
    /* ARM import veneer: ldr ip,[pc,#imm]; ldr pc,[ip] */
    if ((w & 0xFFFFF000) == 0xE59FC000) {
        uint32_t w2 = *(uint32_t *)(s_file + off + 4);
        if ((w2 & 0xFFFFF000) == 0xE59FF000) return 1;
    }
    /* Scheduled ARM prologues where push/branch/ldr-lit is 1-3 insns in */
    for (int k = 1; k <= 3; k++) {
        uint32_t wk = *(uint32_t *)(s_file + off + 4 * k);
        if ((wk & 0xFFFF0000) == 0xE92D0000) return 1;
        if ((wk & 0xFFFF0FFF) == 0xE52D0004) return 1;
        if ((wk & 0xFF000000) == 0xEA000000) return 1;
        if (wk == 0xE12FFF1E) return 1;
        if ((wk & 0x0F7F0000) == 0x051F0000) return 1;
    }
    return 0;
}

static int looks_like_string(uint32_t v) {
    section_t *t = find_section("__TEXT", "__text");
    if (!t) return 0;
    if (v < t->addr || v + 8 >= t->addr + t->size) return 0;
    if (v >= 0x134738 && v < 0x206df8) return 0;
    uint32_t off = v - 0x1000;
    int printable = 0;
    for (int i = 0; i < 64; i++) {
        uint8_t c = s_file[off + i];
        if (c == 0) return printable >= 4;
        if ((c >= 0x20 && c < 0x7f) || c == '\n' || c == '\r' || c == '\t') printable++;
        else return 0;
    }
    return 0;
}

static int is_pointer_value(uint32_t v) {
    if (v < 0x2000 || v >= s_max_vm) return 0;
    /* Reject zero-padding holes at the end of _g_s3e_code and _g_s3e_data */
    if (v >= 0x134738 && v < 0x206df8) return 0;
    if (v >= 0x24b3d8 && v < 0x6f8f00) return 0;
    /* _g_s3e_code .rodata (strings, vtables, typeinfo, static tables) */
    if (v >= 0x112be8 && v < 0x131254) return 1;
    if (sym_has(v) || sym_has(v & ~1u)) return 1;
    if (looks_like_function(v)) return 1;
    if (looks_like_string(v)) return 1;
    if (in_section_range("__TEXT", "__cstring", v)) return 1;
    if (in_section_range("__TEXT", "__const", v)) return 1;
    if (in_section_range("__TEXT", "__symbol_stub4", v)) return 1;
    if (in_section_range("__DATA", "__cfstring", v)) return 1;
    if (in_section_range("__DATA", "__objc_classlist", v)) return 1;
    if (in_section_range("__DATA", "__objc_classrefs", v)) return 1;
    if (in_section_range("__DATA", "__objc_selrefs", v)) return 1;
    if (in_section_range("__DATA", "__objc_superrefs", v)) return 1;
    if (in_section_range("__DATA", "__objc_protolist", v)) return 1;
    if (in_section_range("__DATA", "__objc_imageinfo", v)) return 1;
    if (in_section_range("__DATA", "__data", v)) return 1;
    if (in_section_range("__DATA", "__common", v)) return 1;
    if (in_section_range("__DATA", "__bss", v)) return 1;
    if (in_section_range("__DATA", "__dyld", v)) return 1;
    if (in_section_range("__DATA", "__nl_symbol_ptr", v)) return 1;
    if (in_section_range("__DATA", "__la_symbol_ptr", v)) return 1;
    return 0;
}

typedef struct { uint32_t lo, hi; } skip_range_t;
static skip_range_t s_skip[256];
static int s_nskip = 0;
static int s_skip_active = 0;

static int is_skipped(uint32_t vm) {
    for (int i = 0; i < s_nskip; i++)
        if (vm >= s_skip[i].lo && vm < s_skip[i].hi) return 1;
    return 0;
}
static void add_skip(uint32_t lo, uint32_t hi) {
    if (s_nskip < (int)(sizeof(s_skip)/sizeof(s_skip[0]))) {
        s_skip[s_nskip].lo = lo; s_skip[s_nskip].hi = hi; s_nskip++;
    }
}

/* ---------------- helpers ---------------- */

static section_t *find_section(const char *seg, const char *name) {
    for (int i = 0; i < s_nsegs; i++)
        for (uint32_t j = 0; j < s_segs[i].nsects; j++) {
            section_t *s = &s_segs[i].sects[j];
            if (strncmp(s->segname, seg, 16) == 0 && strncmp(s->sectname, name, 16) == 0)
                return s;
        }
    return NULL;
}

static const char *sym_name(uint32_t idx) {
    if (!s_symtab || idx >= s_symtab->nsyms) return "";
    uint32_t off = s_syms[idx].n_strx;
    if (off >= s_symtab->strsize) return "";
    return s_strtab + off;
}

static uint32_t rebase_val(uint32_t v) {
    if (v >= 0x1000 && v < s_max_vm) return v + g_macho.delta;
    return v;
}

/* ---------------- memory ---------------- */

static uint8_t *alloc_image(uint32_t size) {
    SceKernelAllocMemBlockKernelOpt opt;
    memset(&opt, 0, sizeof(opt));
    opt.size = sizeof(opt);
    opt.attr = 0x1;
    opt.field_C = 0x98000000;
    SceUID id = kuKernelAllocMemBlock("redeg_image", 0x0C20D060, size, &opt);
    if (id < 0) {
        l_warn("kuKernelAllocMemBlock fixed failed (0x%x), trying anywhere", id);
        memset(&opt, 0, sizeof(opt));
        opt.size = sizeof(opt);
        opt.attr = 0x1;
        id = kuKernelAllocMemBlock("redeg_image", 0x0C20D060, size, &opt);
    }
    if (id < 0) {
        l_warn("kuKernelAllocMemBlock failed (0x%x), trying sceKernelAllocMemBlock", id);
        id = sceKernelAllocMemBlock("redeg_image", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW, size, NULL);
    }
    void *base = NULL;
    if (id < 0 || sceKernelGetMemBlockBase(id, &base) < 0) {
        l_fatal("could not allocate %u bytes for the Mach-O image", size);
        return NULL;
    }
    l_info("image block 0x%x at %p (%u bytes)", id, base, size);
    return (uint8_t *)base;
}

/* ---------------- literal rebase ---------------- */

static uint32_t *s_arm_lits = NULL;
static uint32_t s_n_arm_lits = 0;
static uint32_t s_cap_arm_lits = 0;

/* Numeric pools in this exact IPA, verified from their LDR consumers.
 * 0x3ffff is a bound/mask, and 0x2000 is ORR/BIC'd into a flag word.
 * An odd value inside the code range is insufficient evidence of a pointer. */
static int is_numeric_literal(uint32_t vm) {
    static const struct { uint32_t vm, value; } literals[] = {
        {0x03c274, 0x3ffff}, {0x03cb80, 0x3ffff},
        {0x0938e4, 0x2000}, {0x098ca8, 0x3ffff},
        {0x0c47c8, 0x3ffff}, {0x0ed9ec, 0x3ffff},
        {0x100868, 0x3ffff},
    };
    for (unsigned i = 0; i < sizeof(literals) / sizeof(literals[0]); i++)
        if (vm == literals[i].vm &&
            *(const uint32_t *)(s_file + vm - 0x1000) == literals[i].value)
            return 1;
    return 0;
}

static void arm_lit_push(uint32_t v) {
    if (s_n_arm_lits == s_cap_arm_lits) {
        uint32_t cap = s_cap_arm_lits ? s_cap_arm_lits * 2 : 4096;
        uint32_t *p = (uint32_t *)realloc(s_arm_lits, cap * sizeof(uint32_t));
        if (!p) return;
        s_arm_lits = p;
        s_cap_arm_lits = cap;
    }
    s_arm_lits[s_n_arm_lits++] = v;
}

static int arm_lit_has(uint32_t v) {
    int lo = 0, hi = (int)s_n_arm_lits - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (s_arm_lits[mid] == v) return 1;
        if (s_arm_lits[mid] < v) lo = mid + 1; else hi = mid - 1;
    }
    return 0;
}

static void rebase_literals(void) {
    section_t *text = find_section("__TEXT", "__text");
    if (!text) return;
    uint32_t start = text->addr, end = text->addr + text->size;
    uint32_t n = 0;
    s_n_arm_lits = 0;
    for (uint32_t a = start; a < end; a += 4) {
        /* skip crt1's PIC GOT thunks, _g_s3e_code .rodata, and zero padding */
        if (a >= 0x2000 && a < 0x2060) continue;
        if (a >= 0x112be8 && a < 0x131254) continue;
        if (a >= 0x134738 && a < 0x206df8) continue;
        uint32_t insn = *(uint32_t *)(s_file + (a - 0x1000));
        if ((insn & 0x0F7F0000) == 0x051F0000) {
            uint32_t U = (insn >> 23) & 1;
            uint32_t imm = insn & 0xfff;
            uint32_t target = U ? (a + 8 + imm) : (a + 8 - imm);
            if (target >= start && target < end) arm_lit_push(target);
            uint32_t rt = (insn >> 12) & 0xF;
            int is_pic = 0;
            for (uint32_t k = 1; k <= 3; k++) {
                uint32_t w = *(uint32_t *)(s_file + (a + 4 * k - 0x1000));
                if ((w & 0x0FEF0000) == 0x008F0000 && ((w >> 12) & 0xF) == rt) { is_pic = 1; break; }
                if ((w & 0x0E000000) == 0x0A000000) break;   /* b/bl */
                if ((w & 0x0C100000) == 0x04100000) break;   /* ldr/ldrb */
                if ((w & 0x0FFF0FF0) == 0x012FFF10) break;   /* bx */
            }
            if (is_pic) continue;
            if (target >= start && target < end && !is_numeric_literal(target) && !(target >= 0x113140 && target < 0x1300b0)) {
                uint32_t *lit = (uint32_t *)(g_macho.load_base + (target - 0x1000));
                uint32_t v = *lit;
                if (is_pointer_value(v)) { *lit = v + g_macho.delta; n++; }
            }
        }
    }
    if (s_n_arm_lits) {
        qsort(s_arm_lits, s_n_arm_lits, sizeof(uint32_t), cmp_u32);
        uint32_t w = 0;
        for (uint32_t i = 0; i < s_n_arm_lits; i++)
            if (i == 0 || s_arm_lits[i] != s_arm_lits[i - 1]) s_arm_lits[w++] = s_arm_lits[i];
        s_n_arm_lits = w;
    }

    /* Thumb-1 LDR Rt,[pc,#imm8*4] literal pools. */
    for (uint32_t a = start; a + 2 < end; a += 2) {
        if (a >= 0x112be8 && a < 0x131254) continue;
        if (a >= 0x134738 && a < 0x206df8) continue;
        uint16_t h = *(uint16_t *)(s_file + (a - 0x1000));
        if ((h & 0xF800) == 0x4800) {
            uint32_t target = ((a + 4) & ~3u) + ((uint32_t)(h & 0xFF) << 2);
            if (target < start || target + 4 > end) continue;
            if (is_numeric_literal(target)) continue;
            if (target >= 0x113140 && target < 0x1300b0) continue;
            if (arm_lit_has(target)) continue;
            uint32_t rt = (h >> 8) & 7;
            int is_pic = 0;
            for (uint32_t k = 1; k <= 3; k++) {
                uint16_t w = *(uint16_t *)(s_file + (a + 2 * k - 0x1000));
                if ((w & 0xFFF8) == 0x4478 && (w & 7) == rt) { is_pic = 1; break; }
                if ((w & 0xF000) == 0xD000) break;   /* conditional branch */
                if ((w & 0xF800) == 0xE000) break;   /* b */
                if ((w & 0xF800) == 0x4800) break;   /* ldr */
                if ((w & 0xFF00) == 0x4700) break;   /* bx/blx */
            }
            if (is_pic) continue;
            uint32_t *lit = (uint32_t *)(g_macho.load_base + (target - 0x1000));
            uint32_t v = *lit;
            if (is_pointer_value(v)) { *lit = v + g_macho.delta; n++; }
        }
    }
    free(s_arm_lits);
    s_arm_lits = NULL;
    s_cap_arm_lits = 0;
    l_info("rebased %u literal-pool words", n);
}

static void rebase_jump_tables(void) {
    section_t *text = find_section("__TEXT", "__text");
    if (!text) return;
    uint32_t end = text->addr + text->size;
    uint32_t n = 0;
    for (uint32_t a = text->addr + 4; a + 4 < end; a += 4) {
        if (a >= 0x112be8 && a < 0x206df8) continue;
        uint32_t insn = *(uint32_t *)(s_file + (a - 0x1000));
        /* ldr pc, [pc, rX, lsl #2] */
        if ((insn & 0x0FF00000) == 0x07900000 &&
            (insn & 0x000F0000) == 0x000F0000 &&
            (insn & 0x0000F000) == 0x0000F000) {
            uint32_t table = (a + 8) & ~3u;
            for (uint32_t i = 0; i < 512 && table + i * 4 < end; i++) {
                uint32_t orig = *(uint32_t *)(s_file + (table - 0x1000) + i * 4);
                uint32_t dist = (orig > a) ? (orig - a) : (a - orig);
                if (orig >= 0x2000 && orig < end && (orig & 3) == 0 && dist < 0x10000) {
                    uint32_t *w = (uint32_t *)(g_macho.load_base + (table - 0x1000) + i * 4);
                    if (*w < 0x98000000u) { *w = orig + g_macho.delta; n++; }
                } else {
                    break;
                }
            }
        }
    }
    l_info("rebased %u jump-table entries", n);
}

static void rebase_stubs(void) {
    section_t *st = find_section("__TEXT", "__symbol_stub4");
    if (!st) return;
    uint32_t n = 0;
    for (uint32_t off = 0; off + 12 <= st->size; off += 12) {
        uint32_t *lit = (uint32_t *)(g_macho.load_base + (st->addr - 0x1000) + off + 8);
        if (*lit >= 0x2000 && *lit < s_max_vm) { *lit += g_macho.delta; n++; }
    }
    l_info("rebased %u stub literals", n);
}

static void rebase_section(section_t *s, const char *tag) {
    if (!s || s->size == 0) return;
    uint32_t n = 0;
    for (uint32_t off = 0; off + 4 <= s->size; off += 4) {
        uint32_t vm = s->addr + off;
        if (vm >= 0x24b3d8 && vm < 0x6f8f00) continue;
        if (s_skip_active && is_skipped(vm)) continue;
        uint32_t *w = (uint32_t *)(g_macho.load_base + (s->addr - 0x1000) + off);
        if (is_pointer_value(*w)) { *w += g_macho.delta; n++; }
    }
    if (n) l_info("rebased %u words in %s", n, tag);
}
static uint32_t file_u32(uint32_t vm) { return *(uint32_t *)(s_file + (vm - 0x1000)); }

static void collect_ivars(uint32_t ro) {
    uint32_t ivars = file_u32(ro + 28);
    if (ivars < 0x1000 || ivars >= s_max_vm) return;
    uint32_t entsize = file_u32(ivars);
    uint32_t count = file_u32(ivars + 4);
    if (entsize < 20 || entsize > 128 || count > 4096) return;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t entry = ivars + 8 + i * entsize;
        if (entry + 8 >= s_max_vm) break;
        uint32_t offvar = file_u32(entry);
        if (offvar >= 0x1000 && offvar < s_max_vm) add_skip(offvar, offvar + 4);
    }
}

static void rebase_section_all(section_t *s, const char *tag) {
    if (!s || s->size == 0) return;
    uint32_t n = 0;
    for (uint32_t off = 0; off + 4 <= s->size; off += 4) {
        uint32_t *w = (uint32_t *)(g_macho.load_base + (s->addr - 0x1000) + off);
        if (*w >= 0x2000 && *w < s_max_vm) { *w += g_macho.delta; n++; }
    }
    if (n) l_info("rebased %u words in %s (all)", n, tag);
}

static void build_data_skip_ranges(void) {
    s_nskip = 0;
    /* _g_s3e_data 96-byte XE3U module header and packed byte constant table */
    add_skip(0x234400, 0x234460);
    add_skip(0x235c38, 0x235cd8);
    /* IwMath's uint16_t sine quarter-wave (1025 entries + alignment),
     * followed by the 513-entry int32_t atan table. Neither contains
     * pointers. In particular, sine[2:4] packs as 0x0012000c and the
     * generic pointer heuristic mistakes it for an address in .rodata.
     * Rebasing it makes sin(2/3) return 61452/38929 instead of 12/18,
     * causing quaternion slerp to explode before geometry reaches GL. */
    add_skip(0x235e9c, 0x2366a0);
    add_skip(0x2366a0, 0x236ea4);
    l_info("[graphics] preserving IwMath sine/atan numeric tables");
    section_t *cl = find_section("__DATA", "__objc_classlist");
    if (!cl) return;
    for (uint32_t i = 0; i < cl->size / 4; i++) {
        uint32_t cls = *(uint32_t *)(s_file + cl->offset + i * 4);
        if (cls < 0x1000 || cls >= s_max_vm) continue;
        uint32_t meta = file_u32(cls);
        if (meta >= 0x1000 && meta < s_max_vm) {
            uint32_t ro = file_u32(meta + 16);
            if (ro >= 0x1000 && ro < s_max_vm) { add_skip(ro, ro + 12); collect_ivars(ro); }
        }
        uint32_t ro = file_u32(cls + 16);
        if (ro >= 0x1000 && ro < s_max_vm) { add_skip(ro, ro + 12); collect_ivars(ro); }
    }
    l_info("data rebase skip ranges: %d", s_nskip);
}

/* ---------------- symbols ---------------- */

extern uint32_t redeg_resolve_symbol(const char *name);
extern void redeg_missing_import(void);

static void bind_pointers(void) {
    section_t *nl = find_section("__DATA", "__nl_symbol_ptr");
    section_t *la = find_section("__DATA", "__la_symbol_ptr");
    section_t *sects[2] = {nl, la};
    uint32_t unresolved = 0;
    for (int k = 0; k < 2; k++) {
        section_t *s = sects[k];
        if (!s) continue;
        uint32_t count = s->size / 4;
        for (uint32_t i = 0; i < count; i++) {
            uint32_t *slot = (uint32_t *)(g_macho.load_base + (s->addr - 0x1000) + i * 4);
            uint32_t idx = *(uint32_t *)(s_file + (s_dysym->indirectsymoff + (s->reserved1 + i) * 4));
            if (idx == 0x80000000u) { /* LOCAL */
                *slot = rebase_val(*slot);
            } else if (idx == 0x80000001u) { /* ABS */
                /* leave */
            } else {
                const char *name = sym_name(idx);
                uint32_t v = redeg_resolve_symbol(name);
                if (v == 0) {
                    unresolved++;
                    l_warn("unresolved import %s", name);
                    v = (uint32_t)(uintptr_t)redeg_missing_import;
                }
                *slot = v;
            }
        }
    }
    l_info("bound symbol pointers, unresolved=%u", unresolved);
}

static void apply_extrel(void) {
    if (!s_dysym || s_dysym->nextrel == 0) return;
    relocation_info_t *r = (relocation_info_t *)(s_file + s_dysym->extreloff);
    int ok = 0;
    for (uint32_t i = 0; i < s_dysym->nextrel; i++) {
        uint32_t addr = (uint32_t)r[i].r_address;
        uint32_t symnum = r[i].r_info & 0xffffff;
        uint32_t type = (r[i].r_info >> 28) & 0xf;
        if (type != 0) { l_warn("extrel type %u at 0x%x unsupported", type, addr); continue; }
        const char *name = sym_name(symnum);
        uint32_t v = redeg_resolve_symbol(name);
        if (!v) { l_warn("extrel unresolved %s at 0x%x", name, addr); continue; }
        *(uint32_t *)(g_macho.load_base + (addr - 0x1000)) = v;
        ok++;
    }
    l_info("applied %d/%u external relocations", ok, s_dysym->nextrel);
}

/* _g_s3e_code .rodata (0x112be8..0x131254) is embedded inside __TEXT,__text.
 * Its pointer-bearing sub-sections are:
 *   0x113140..0x113db4 : IwClassFactory / enum / property tables
 *   0x114e64..0x115200 : C++ pointer-to-member-function & callback tables
 *   0x115560..0x118c08 : Itanium C++ ABI _ZTI* std::type_info structs
 *   0x11f26c..0x1300b0 : Itanium C++ ABI _ZTV* primary & secondary vtables */
static void rebase_rodata_range(uint32_t lo, uint32_t hi, const char *tag) {
    uint32_t n = 0;
    for (uint32_t vm = lo; vm + 4 <= hi; vm += 4) {
        if (vm == 0x113518) continue; /* 0x00080808 packed byte constant */
        uint32_t *w = (uint32_t *)(g_macho.load_base + (vm - 0x1000));
        if (*w >= 0x35600 && *w < 0x131254) {
            *w += g_macho.delta;
            n++;
        }
    }
    l_info("rebased %u words in .rodata %s", n, tag);
}

static void rebase_rodata(void) {
    rebase_rodata_range(0x113140, 0x113db4, "tables");
    rebase_rodata_range(0x114e64, 0x115200, "ptmf");
    rebase_rodata_range(0x115560, 0x118c08, "typeinfo");
    rebase_rodata_range(0x11f26c, 0x1300b0, "vtables");
}

/* ---------------- load ---------------- */

static void apply_patches(void);

int macho_load(const char *path, macho_image_t *out) {
    memset(out, 0, sizeof(*out));
    FILE *f = fopen(path, "rb");
    if (!f) { l_fatal("cannot open %s", path); return -1; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return -1; }
    s_filesize = (size_t)sz;
    s_file = (uint8_t *)malloc(s_filesize);
    if (!s_file) { fclose(f); return -1; }
    if (fread(s_file, 1, s_filesize, f) != s_filesize) { fclose(f); return -1; }
    fclose(f);

    if (*(uint32_t *)s_file != MH_MAGIC) {
        l_fatal("not a Mach-O ARM binary (magic 0x%08x)", *(uint32_t *)s_file);
        return -1;
    }
    uint32_t ncmds = *(uint32_t *)(s_file + 16);
    uint32_t off = 28;
    s_nsegs = 0;
    s_max_vm = 0;

    for (uint32_t i = 0; i < ncmds; i++) {
        load_command_t *lc = (load_command_t *)(s_file + off);
        if (lc->cmd == LC_SEGMENT) {
            segment_command_t *sc = (segment_command_t *)lc;
            if (strncmp(sc->segname, "__PAGEZERO", 16) != 0) {
                seg_t *sg = &s_segs[s_nsegs++];
                memcpy(sg->segname, sc->segname, 16);
                sg->vmaddr = sc->vmaddr;
                sg->vmsize = sc->vmsize;
                sg->fileoff = sc->fileoff;
                sg->filesize = sc->filesize;
                sg->nsects = sc->nsects;
                sg->sects = (section_t *)((uint8_t *)sc + sizeof(segment_command_t));
                uint32_t end = sc->vmaddr + sc->vmsize;
                if (end > s_max_vm) s_max_vm = end;
            }
        } else if (lc->cmd == LC_SYMTAB) {
            s_symtab = (symtab_command_t *)lc;
        } else if (lc->cmd == LC_DYSYMTAB) {
            s_dysym = (dysymtab_command_t *)lc;
        }
        off += lc->cmdsize;
    }

    s_syms = (nlist_t *)(s_file + s_symtab->symoff);
    s_strtab = (const char *)(s_file + s_symtab->stroff);

    s_sym_addrs = (uint32_t *)malloc(s_symtab->nsyms * sizeof(uint32_t));
    s_nsym_addrs = 0;
    for (uint32_t i = 0; i < s_symtab->nsyms; i++) {
        if ((s_syms[i].n_type & 0x0e) == 0x0e && s_syms[i].n_value)
            s_sym_addrs[s_nsym_addrs++] = s_syms[i].n_value;
    }
    qsort(s_sym_addrs, s_nsym_addrs, sizeof(uint32_t), cmp_u32);
    l_info("symbols for pointer check: %d", s_nsym_addrs);

    uint32_t image_size = (s_max_vm - 0x1000 + 0xfff) & ~0xfffu;
    l_info("Mach-O: %d segments, vmsize 0x%x, image 0x%x", s_nsegs, s_max_vm, image_size);
    uint8_t *mem = alloc_image(image_size);
    if (!mem) return -1;

    g_macho.load_base = (uint32_t)(uintptr_t)mem;
    g_macho.delta = g_macho.load_base - 0x1000;
    g_macho.image_size = image_size;

    memset(mem, 0, image_size);
    for (int i = 0; i < s_nsegs; i++) {
        seg_t *sg = &s_segs[i];
        if (sg->filesize && sg->fileoff + sg->filesize <= s_filesize) {
            memcpy(mem + (sg->vmaddr - 0x1000), s_file + sg->fileoff, sg->filesize);
        }
    }

    /* find _main */
    for (uint32_t i = 0; i < s_symtab->nsyms; i++) {
        const char *n = sym_name(i);
        if (strcmp(n, "_main") == 0) { g_macho.entry = s_syms[i].n_value; break; }
    }
    section_t *cl = find_section("__DATA", "__objc_classlist");
    if (cl) {
        g_macho.classlist = g_macho.load_base + (cl->addr - 0x1000);
        g_macho.nclasses = (int)(cl->size / 4);
    }
    section_t *text = find_section("__TEXT", "__text");
    if (text) { g_macho.text_addr = text->addr; g_macho.text_size = text->size; }
    section_t *stubs = find_section("__TEXT", "__symbol_stub4");
    if (stubs) { g_macho.lc_symbol_stub = stubs->addr; g_macho.lc_symbol_stub_size = stubs->size; }

    l_info("load_base=0x%08x delta=0x%08x main=0x%08x classes=%d",
           g_macho.load_base, g_macho.delta, g_macho.entry, g_macho.nclasses);

    apply_extrel();
    bind_pointers();
    rebase_rodata();
    rebase_literals();
    rebase_jump_tables();
    rebase_stubs();

    /* pointer sections */
    build_data_skip_ranges();
    s_skip_active = 1;
    rebase_section(find_section("__DATA", "__data"), "__data");
    s_skip_active = 0;
    rebase_section_all(find_section("__DATA", "__objc_classlist"), "classlist");
    rebase_section_all(find_section("__DATA", "__objc_classrefs"), "classrefs");
    rebase_section_all(find_section("__DATA", "__objc_superrefs"), "superrefs");
    rebase_section_all(find_section("__DATA", "__objc_selrefs"), "selrefs");
    rebase_section_all(find_section("__DATA", "__objc_protolist"), "protolist");
    rebase_section_all(find_section("__DATA", "__cfstring"), "cfstring");
    rebase_section_all(find_section("__DATA", "__dyld"), "__dyld");

    apply_patches();

    /* __TEXT becomes RX at runtime; __DATA must stay writable. */
    uint32_t data_vm = 0x233000;
    for (int i = 0; i < s_nsegs; i++)
        if (strncmp(s_segs[i].segname, "__DATA", 16) == 0) data_vm = s_segs[i].vmaddr;
    int pr = kuKernelMemProtect((void *)(uintptr_t)g_macho.load_base,
                                data_vm - 0x1000, KU_KERNEL_PROT_READ | KU_KERNEL_PROT_EXEC);
    l_info("text protect RX -> %d", pr);

    macho_flush();
    *out = g_macho;
    return 0;
}

typedef struct { uint32_t vmaddr; uint32_t word; uint32_t target; int is_branch; } macho_patch_t;
static macho_patch_t s_patches[128];
static int s_npatches = 0;

void macho_add_patch(uint32_t vmaddr, uint32_t word) {
    if (s_npatches < (int)(sizeof(s_patches) / sizeof(s_patches[0]))) {
        s_patches[s_npatches].vmaddr = vmaddr;
        s_patches[s_npatches].word = word;
        s_patches[s_npatches].is_branch = 0;
        s_npatches++;
    }
}

void macho_add_branch(uint32_t vmaddr, uint32_t target) {
    if (s_npatches < (int)(sizeof(s_patches) / sizeof(s_patches[0]))) {
        s_patches[s_npatches].vmaddr = vmaddr;
        s_patches[s_npatches].target = target;
        s_patches[s_npatches].is_branch = 1;
        s_npatches++;
    }
}

static void apply_patches(void) {
    for (int i = 0; i < s_npatches; i++) {
        uint32_t *w = (uint32_t *)(uintptr_t)(s_patches[i].vmaddr + g_macho.delta);
        if (s_patches[i].is_branch) {
            uint32_t from = s_patches[i].vmaddr + g_macho.delta;
            int32_t off = (int32_t)(s_patches[i].target - (from + 8));
            *w = 0xEA000000u | (((uint32_t)(off >> 2)) & 0x00FFFFFFu);
            l_info("guest branch 0x%08x -> 0x%08x", from, s_patches[i].target);
        } else {
            *w = s_patches[i].word;
            l_info("guest patch 0x%08x = 0x%08x", s_patches[i].vmaddr, s_patches[i].word);
        }
    }
}

void macho_flush(void) {
    kuKernelFlushCaches((const void *)(uintptr_t)g_macho.load_base, g_macho.image_size);
}

const char *macho_symbol_name(uint32_t addr) {
    uint32_t orig = addr - g_macho.delta;
    for (uint32_t i = 0; i < s_symtab->nsyms; i++) {
        if (s_syms[i].n_value == orig) return sym_name(i);
    }
    return "?";
}
