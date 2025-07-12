#include "user.h"
int main(int argc, char *argv[]) {
  struct memstat ms;
  if (memstat(&ms) < 0) {
    printf("memstat failed\n");
    exit(1);
  }
  printf("Total pages: %lu\n", ms.total_pages);
  printf("Free pages: %lu\n", ms.free_pages);
  printf("Used pages: %lu\n", ms.used_pages);
  printf("Total memory: %lu bytes\n", ms.total_memory);
  printf("Free memory: %lu bytes\n", ms.free_memory);
  printf("Used memory: %lu bytes\n", ms.used_memory);
  printf("Page size: %lu bytes\n", ms.page_size);

  exit(0);
}
