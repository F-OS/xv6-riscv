// sym2bin.c
// Usage: ./sym2bin input.txt output.bin

#define _GNU_SOURCE // for getline() and strdup()
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct sym {
  uint64_t addr;
  char *name;
};

static int cmp_sym_by_addr(const void *A, const void *B) {
  const struct sym *a = A;
  const struct sym *b = B;
  if (a->addr < b->addr)
    return -1;
  if (a->addr > b->addr)
    return 1;
  return 0;
}

int main(int argc, char **argv) {

  if (argc >= 2 && strcmp(argv[1], "-d") == 0) {
    // Generate dummy file with 0 symbols
    const char *outpath = argv[2];
    FILE *fout = fopen(outpath, "wb");
    if (!fout) {
      perror("fopen output");
      return 1;
    }
    // Write magic header
    const char magic[] = "KSYM\0";
    if (fwrite(magic, sizeof magic - 1, 1, fout) != 1) {
      perror("fwrite magic");
      fclose(fout);
      return 1;
    }
    // write count as 32‑bit little‑endian
    int32_t count32 = 0;
    if (fwrite(&count32, sizeof count32, 1, fout) != 1) {
      perror("fwrite count");
      fclose(fout);
      return 1;
    }
    fclose(fout);
    fprintf(stderr, "Wrote 0 symbols to %s\n", outpath);

    return 0;
  }

  if (argc != 3) {
    fprintf(stderr, "Usage: %s <input.txt> <output.bin>\n", argv[0]);
    return 1;
  }

  const char *inpath = argv[1];
  const char *outpath = argv[2];

  FILE *fin = fopen(inpath, "r");
  if (!fin) {
    perror("fopen input");
    return 1;
  }

  struct sym *syms = NULL;
  size_t cap = 0, cnt = 0;
  char *line = NULL;
  size_t linelen = 0;

  while (1) {
    ssize_t nread = getline(&line, &linelen, fin);
    if (nread < 0)
      break;

    // strip newline
    if (nread > 0 && line[nread - 1] == '\n')
      line[--nread] = '\0';

    // parse "<hexaddr> <name>"
    char *tok = strtok(line, " \t");
    if (!tok)
      continue; // empty line
    errno = 0;
    uint64_t addr = strtoull(tok, NULL, 16);
    if (errno) {
      fprintf(stderr, "invalid address \"%s\"\n", tok);
      continue;
    }
    tok = strtok(NULL, " \t");
    if (!tok) {
      fprintf(stderr, "missing name on line: %s\n", line);
      continue;
    }

    // grow array if needed
    if (cnt >= cap) {
      size_t newcap = cap ? cap * 2 : 128;
      struct sym *tmp = realloc(syms, newcap * sizeof *syms);
      if (!tmp) {
        perror("realloc");
        free(syms);
        fclose(fin);
        return 1;
      }
      syms = tmp;
      cap = newcap;
    }

    // record it
    syms[cnt].addr = addr;
    syms[cnt].name = strdup(tok);
    if (!syms[cnt].name) {
      perror("strdup");
      // cleanup...
      fclose(fin);
      for (size_t i = 0; i < cnt; i++)
        free(syms[i].name);
      free(syms);
      return 1;
    }
    cnt++;
  }
  free(line);
  fclose(fin);

  // sort by address
  qsort(syms, cnt, sizeof *syms, cmp_sym_by_addr);

  // open output
  FILE *fout = fopen(outpath, "wb");
  if (!fout) {
    perror("fopen output");
    goto cleanup;
  }

  // Write magic header
  const char magic[] = "KSYM\0";
  if (fwrite(magic, sizeof magic - 1, 1, fout) != 1) {
    perror("fwrite magic");
    fclose(fout);
    goto cleanup;
  }

  // write count as 32‑bit little‑endian
  int32_t count32 = (int32_t)cnt;
  if (fwrite(&count32, sizeof count32, 1, fout) != 1) {
    perror("fwrite count");
    fclose(fout);
    goto cleanup;
  }

  // write each symbol: uint64_t addr; then name padded to 64 bytes with '\0'
  // (or truncated if longer)
  for (size_t i = 0; i < cnt; i++) {
    if (fwrite(&syms[i].addr, sizeof syms[i].addr, 1, fout) != 1) {
      perror("fwrite addr");
      fclose(fout);
      goto cleanup;
    }
    size_t namelen = strlen(syms[i].name) + 1;
    if (namelen > 64) {
      fprintf(stderr,
              "Warning: symbol name \"%s\" too long, truncating to 64 bytes\n",
              syms[i].name);
      namelen = 64; // truncate
    }
    char namebuf[64] = {0};                      // zero-initialize
    strncpy(namebuf, syms[i].name, namelen - 1); // copy
    namebuf[namelen - 1] = '\0';                 // ensure null-termination
    if (fwrite(namebuf, 64, 1, fout) != 1) {
      perror("fwrite name");
      fclose(fout);
      goto cleanup;
    }
  }

  fclose(fout);
  fprintf(stderr, "Wrote %zu symbols to %s\n", cnt, outpath);

cleanup:
  for (size_t i = 0; i < cnt; i++)
    free(syms[i].name);
  free(syms);
  return 0;
}
