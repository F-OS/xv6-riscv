#ifndef LOG_H
#define LOG_H
#include "bio.h"
#include "fs.h"

void initlog(int dev, struct superblock *sb);
void log_write(struct buf *b);
void begin_op(void);
void end_op(void);

#endif // LOG_H