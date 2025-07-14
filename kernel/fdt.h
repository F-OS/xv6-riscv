// See LICENSE for license details.

#ifndef FDT_H
#define FDT_H
#include "kernel/param.h"
#include "types.h"

#define FDT_MAGIC 0xd00dfeed
#define FDT_VERSION 17

struct fdt_header {
  uint32 magic;
  uint32 totalsize;
  uint32 off_dt_struct;
  uint32 off_dt_strings;
  uint32 off_mem_rsvmap;
  uint32 version;
  uint32 last_comp_version; /* <= 17 */
  uint32 boot_cpuid_phys;
  uint32 size_dt_strings;
  uint32 size_dt_struct;
};

#define FDT_BEGIN_NODE 1
#define FDT_END_NODE 2
#define FDT_PROP 3
#define FDT_NOP 4
#define FDT_END 9

struct fdt_scan_node {
  const struct fdt_scan_node *parent;
  const char *name;
  int address_cells;
  int size_cells;
};

struct fdt_scan_prop {
  const struct fdt_scan_node *node;
  const char *name;
  uint32 *value;
  int len; // in bytes of value
};

struct fdt_cb {
  void (*open)(const struct fdt_scan_node *node, void *extra);
  void (*prop)(const struct fdt_scan_prop *prop, void *extra);
  void (*done)(const struct fdt_scan_node *node,
               void *extra); // last property was seen
  int (*close)(const struct fdt_scan_node *node,
               void *extra); // -1 => delete the node + children
  void *extra;
};

// Scan the contents of FDT
void fdt_scan(uint64 fdt, const struct fdt_cb *cb);
uint32 fdt_size(uint64 fdt);

// Extract fields
const uint32 *fdt_get_address(const struct fdt_scan_node *node,
                              const uint32 *base, uint64 *value);
const uint32 *fdt_get_size(const struct fdt_scan_node *node, const uint32 *base,
                           uint64 *value);
uint32 fdt_get_value(const struct fdt_scan_prop *prop, uint32 index);
int fdt_string_list_index(const struct fdt_scan_prop *prop,
                          const char *str); // -1 if not found

// Setup memory+clint+plic
void query_mem(uint64 fdt);
void query_harts(uint64 fdt);
void query_plic(uint64 fdt);
void query_clint(uint64 fdt);
void query_chosen(uint64 fdt);

// Remove information from FDT
void filter_harts(uint64 fdt, long *disabled_hart_mask);
void filter_plic(uint64 fdt);
void filter_compat(uint64 fdt, const char *compat);

// The hartids of available harts
extern uint64 hart_mask;


void probe_fdt(uint64 fdt);
extern uint64 mem_size;
#endif
