#pragma once
#include "buffer.h"

void disk_init(void);
void disk_read(struct buffer_head *b);
void disk_write(struct buffer_head *b);
void disk_intr(void);

