#ifndef PIPE_H
#define PIPE_H
#include "file.h"
int pipealloc(struct file **f0, struct file **f1);
void pipeclose(struct pipe *pi, int writable);
int piperead(struct pipe *pi, uint64 addr, int n);
int pipewrite(struct pipe *pi, uint64 addr, int n);
#endif