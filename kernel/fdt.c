// See LICENSE for license details.

#include "fdt.h"
#include "kernel/param.h"
#include "kernel/printf.h"
#include "kernel/riscv.h"
#include "kernel/string.h"
#include "kernel/types.h"



uint32 hart_phandles[NCPU] = {0};
uint64 hart_mask = 0;
uint64 mem_size = 0;


#define assert(x)                                                              \
  do {                                                                         \
    if (!(x)) {                                                                \
      panic("assertion failed: " #x);                                          \
    }                                                                          \
  } while (0)

static inline uint32 bswap(uint32 x) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
  uint32 y = (x & 0x00FF00FF) << 8 | (x & 0xFF00FF00) >> 8;
  uint32 z = (y & 0x0000FFFF) << 16 | (y & 0xFFFF0000) >> 16;
  return z;
#else
  /* No need to swap on big endian */
  return x;
#endif
}

static inline int isstring(char c) {
  if (c >= 'A' && c <= 'Z')
    return 1;
  if (c >= 'a' && c <= 'z')
    return 1;
  if (c >= '0' && c <= '9')
    return 1;
  if (c == '\0' || c == ' ' || c == ',' || c == '-')
    return 1;
  return 0;
}

static uint32 *fdt_scan_helper(uint32 *lex, const char *strings,
                               struct fdt_scan_node *node,
                               const struct fdt_cb *cb) {
  struct fdt_scan_node child;
  struct fdt_scan_prop prop;
  int last = 0;

  child.parent = node;
  // these are the default cell counts, as per the FDT spec
  child.address_cells = 2;
  child.size_cells = 1;
  prop.node = node;

  while (1) {
    switch (bswap(lex[0])) {
    case FDT_NOP: {
      lex += 1;
      break;
    }
    case FDT_PROP: {
      assert(!last);
      prop.name = strings + bswap(lex[2]);
      prop.len = bswap(lex[1]);
      prop.value = lex + 3;
      if (node && !strcmp(prop.name, "#address-cells")) {
        node->address_cells = bswap(lex[3]);
      }
      if (node && !strcmp(prop.name, "#size-cells")) {
        node->size_cells = bswap(lex[3]);
      }
      lex += 3 + (prop.len + 3) / 4;
      cb->prop(&prop, cb->extra);
      break;
    }
    case FDT_BEGIN_NODE: {
      uint32 *lex_next;
      if (!last && node && cb->done)
        cb->done(node, cb->extra);
      last = 1;
      child.name = (const char *)(lex + 1);
      if (cb->open)
        cb->open(&child, cb->extra);
      lex_next = fdt_scan_helper(lex + 2 + strlen(child.name) / 4, strings,
                                 &child, cb);
      if (cb->close && cb->close(&child, cb->extra) == -1)
        while (lex != lex_next)
          *lex++ = bswap(FDT_NOP);
      lex = lex_next;
      break;
    }
    case FDT_END_NODE: {
      if (!last && node && cb->done)
        cb->done(node, cb->extra);
      return lex + 1;
    }
    default: { // FDT_END
      if (!last && node && cb->done)
        cb->done(node, cb->extra);
      return lex;
    }
    }
  }
}

void fdt_scan(uint64 fdt, const struct fdt_cb *cb) {
  struct fdt_header *header = (struct fdt_header *)fdt;

  // Only process FDT that we understand
  if (bswap(header->magic) != FDT_MAGIC ||
      bswap(header->last_comp_version) > FDT_VERSION)
    return;

  const char *strings = (const char *)(fdt + bswap(header->off_dt_strings));
  uint32 *lex = (uint32 *)(fdt + bswap(header->off_dt_struct));

  fdt_scan_helper(lex, strings, 0, cb);
}

uint32 fdt_size(uint64 fdt) {
  struct fdt_header *header = (struct fdt_header *)fdt;

  // Only process FDT that we understand
  if (bswap(header->magic) != FDT_MAGIC ||
      bswap(header->last_comp_version) > FDT_VERSION)
    return 0;
  return bswap(header->totalsize);
}

const uint32 *fdt_get_address(const struct fdt_scan_node *node,
                              const uint32 *value, uint64 *result) {
  *result = 0;
  for (int cells = node->address_cells; cells > 0; --cells)
    *result = (*result << 32) + bswap(*value++);
  return value;
}

const uint32 *fdt_get_size(const struct fdt_scan_node *node,
                           const uint32 *value, uint64 *result) {
  *result = 0;
  for (int cells = node->size_cells; cells > 0; --cells)
    *result = (*result << 32) + bswap(*value++);
  return value;
}

uint32 fdt_get_value(const struct fdt_scan_prop *prop, uint32 index) {
  return bswap(prop->value[index]);
}

int fdt_string_list_index(const struct fdt_scan_prop *prop, const char *str) {
  const char *list = (const char *)prop->value;
  const char *end = list + prop->len;
  int index = 0;
  while (end - list > 0) {
    if (!strcmp(list, str))
      return index;
    ++index;
    list += strlen(list) + 1;
  }
  return -1;
}

//////////////////////////////////////////// MEMORY SCAN
////////////////////////////////////////////

struct mem_scan {
  int memory;
  const uint32 *reg_value;
  int reg_len;
};

static void mem_open(const struct fdt_scan_node *node, void *extra) {
  struct mem_scan *scan = (struct mem_scan *)extra;
  memset(scan, 0, sizeof(*scan));
}

static void mem_prop(const struct fdt_scan_prop *prop, void *extra) {
  struct mem_scan *scan = (struct mem_scan *)extra;
  if (!strcmp(prop->name, "device_type") &&
      !strcmp((const char *)prop->value, "memory")) {
    scan->memory = 1;
  } else if (!strcmp(prop->name, "reg")) {
    scan->reg_value = prop->value;
    scan->reg_len = prop->len;
  }
}

static void mem_done(const struct fdt_scan_node *node, void *extra) {
  struct mem_scan *scan = (struct mem_scan *)extra;
  const uint32 *value = scan->reg_value;
  const uint32 *end = value + scan->reg_len / 4;
  uint64 self = (uint64)mem_done;

  if (!scan->memory)
    return;
  assert(scan->reg_value && scan->reg_len % 4 == 0);

  while (end - value > 0) {
    uint64 base, size;
    value = fdt_get_address(node->parent, value, &base);
    value = fdt_get_size(node->parent, value, &size);
    if (base <= self && self <= base + size) {
      mem_size = size;
    }
  }
  assert(end == value);
}

void query_mem(uint64 fdt) {
  struct fdt_cb cb;
  struct mem_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = mem_open;
  cb.prop = mem_prop;
  cb.done = mem_done;
  cb.extra = &scan;

  mem_size = 0;
  fdt_scan(fdt, &cb);
  assert(mem_size > 0);
}

///////////////////////////////////////////// HART SCAN
/////////////////////////////////////////////

struct hart_scan {
  const struct fdt_scan_node *cpu;
  int hart;
  const struct fdt_scan_node *controller;
  int cells;
  uint32 phandle;
};

static void hart_open(const struct fdt_scan_node *node, void *extra) {
  struct hart_scan *scan = (struct hart_scan *)extra;
  if (!scan->cpu) {
    scan->hart = -1;
  }
  if (!scan->controller) {
    scan->cells = 0;
    scan->phandle = 0;
  }
}

static void hart_prop(const struct fdt_scan_prop *prop, void *extra) {
  struct hart_scan *scan = (struct hart_scan *)extra;
  if (!strcmp(prop->name, "device_type") &&
      !strcmp((const char *)prop->value, "cpu")) {
    assert(!scan->cpu);
    scan->cpu = prop->node;
  } else if (!strcmp(prop->name, "interrupt-controller")) {
    assert(!scan->controller);
    scan->controller = prop->node;
  } else if (!strcmp(prop->name, "#interrupt-cells")) {
    scan->cells = bswap(prop->value[0]);
  } else if (!strcmp(prop->name, "phandle")) {
    scan->phandle = bswap(prop->value[0]);
  } else if (!strcmp(prop->name, "reg")) {
    uint64 reg;
    fdt_get_address(prop->node->parent, prop->value, &reg);
    scan->hart = reg;
  }
}

static void hart_done(const struct fdt_scan_node *node, void *extra) {
  struct hart_scan *scan = (struct hart_scan *)extra;

  if (scan->cpu == node) {
    assert(scan->hart >= 0);
  }

  if (scan->controller == node && scan->cpu) {
    assert(scan->phandle > 0);
    assert(scan->cells == 1);

    if (scan->hart < NCPU) {
      hart_phandles[scan->hart] = scan->phandle;
      hart_mask |= 1 << scan->hart;
    }
  }
}

static int hart_close(const struct fdt_scan_node *node, void *extra) {
  struct hart_scan *scan = (struct hart_scan *)extra;
  if (scan->cpu == node)
    scan->cpu = 0;
  if (scan->controller == node)
    scan->controller = 0;
  return 0;
}

void query_harts(uint64 fdt) {
  struct fdt_cb cb;
  struct hart_scan scan;

  memset(&cb, 0, sizeof(cb));
  memset(&scan, 0, sizeof(scan));
  cb.open = hart_open;
  cb.prop = hart_prop;
  cb.done = hart_done;
  cb.close = hart_close;
  cb.extra = &scan;

  fdt_scan(fdt, &cb);
}

///////////////////////////////////////////// CLINT SCAN
////////////////////////////////////////////

struct clint_scan {
  int compat;
  uint64 reg;
  const uint32 *int_value;
  int int_len;
  int done;
};

static void clint_open(const struct fdt_scan_node *node, void *extra) {
  struct clint_scan *scan = (struct clint_scan *)extra;
  scan->compat = 0;
  scan->reg = 0;
  scan->int_value = 0;
}

static void clint_prop(const struct fdt_scan_prop *prop, void *extra) {
  struct clint_scan *scan = (struct clint_scan *)extra;
  if (!strcmp(prop->name, "compatible") &&
      fdt_string_list_index(prop, "riscv,clint0") >= 0) {
    scan->compat = 1;
  } else if (!strcmp(prop->name, "reg")) {
    fdt_get_address(prop->node->parent, prop->value, &scan->reg);
  } else if (!strcmp(prop->name, "interrupts-extended")) {
    scan->int_value = prop->value;
    scan->int_len = prop->len;
  }
}

static void clint_done(const struct fdt_scan_node *node, void *extra) {
  struct clint_scan *scan = (struct clint_scan *)extra;
  const uint32 *value = scan->int_value;
  const uint32 *end = value + scan->int_len / 4;

  if (!scan->compat)
    return;
  assert(scan->reg != 0);
  assert(scan->int_value && scan->int_len % 16 == 0);
  assert(!scan->done); // only one clint

  scan->done = 1;

  for (int index = 0; end - value > 0; ++index) {
    uint32 phandle = bswap(value[0]);
    int hart;
    for (hart = 0; hart < NCPU; ++hart)
      if (hart_phandles[hart] == phandle)
        break;
    value += 4;
  }
}

void query_clint(uint64 fdt) {
  struct fdt_cb cb;
  struct clint_scan scan;

  memset(&cb, 0, sizeof(cb));
  cb.open = clint_open;
  cb.prop = clint_prop;
  cb.done = clint_done;
  cb.extra = &scan;

  scan.done = 0;
  fdt_scan(fdt, &cb);
  assert(scan.done);
}

//////////////////////////////////////////// HART FILTER
///////////////////////////////////////////

struct hart_filter {
  int compat;
  int hart;
  char *status;
  char *mmu_type;
  long *disabled_hart_mask;
};

static void hart_filter_open(const struct fdt_scan_node *node, void *extra) {
  struct hart_filter *filter = (struct hart_filter *)extra;
  filter->status = NULL;
  filter->mmu_type = NULL;
  filter->compat = 0;
  filter->hart = -1;
}

static void hart_filter_prop(const struct fdt_scan_prop *prop, void *extra) {
  struct hart_filter *filter = (struct hart_filter *)extra;
  if (!strcmp(prop->name, "device_type") &&
      !strcmp((const char *)prop->value, "cpu")) {
    filter->compat = 1;
  } else if (!strcmp(prop->name, "reg")) {
    uint64 reg;
    fdt_get_address(prop->node->parent, prop->value, &reg);
    filter->hart = reg;
  } else if (!strcmp(prop->name, "status")) {
    filter->status = (char *)prop->value;
  } else if (!strcmp(prop->name, "mmu-type")) {
    filter->mmu_type = (char *)prop->value;
  }
}

static bool hart_filter_mask(const struct hart_filter *filter) {
  if (filter->mmu_type == NULL)
    return true;
  if (strcmp(filter->status, "okay"))
    return true;
#if __riscv_xlen == 32
  if (!strcmp(filter->mmu_type, "riscv,sv32"))
    return false;
#else
  if (!strcmp(filter->mmu_type, "riscv,sv39"))
    return false;
  if (!strcmp(filter->mmu_type, "riscv,sv48"))
    return false;
  if (!strcmp(filter->mmu_type, "riscv,sv57"))
    return false;
#endif
  printf("hart_filter_mask saw unknown hart type: status=\"%s\", "
         "mmu_type=\"%s\"\n",
         filter->status, filter->mmu_type);
  return true;
}

static void hart_filter_done(const struct fdt_scan_node *node, void *extra) {
  struct hart_filter *filter = (struct hart_filter *)extra;

  if (!filter->compat)
    return;
  assert(filter->status);
  assert(filter->hart >= 0);

  if (hart_filter_mask(filter)) {
    strcpy(filter->status, "masked");
    uint32 *len = (uint32 *)filter->status;
    len[-2] = bswap(strlen("masked") + 1);
    *filter->disabled_hart_mask |= (1 << filter->hart);
  }
}

void filter_harts(uint64 fdt, long *disabled_hart_mask) {
  struct fdt_cb cb;
  struct hart_filter filter;

  memset(&cb, 0, sizeof(cb));
  cb.open = hart_filter_open;
  cb.prop = hart_filter_prop;
  cb.done = hart_filter_done;
  cb.extra = &filter;

  filter.disabled_hart_mask = disabled_hart_mask;
  *disabled_hart_mask = 0;
  fdt_scan(fdt, &cb);
}

void probe_fdt(uint64 fdt) {
  query_mem(fdt);
  query_harts(fdt);
  query_clint(fdt);
}