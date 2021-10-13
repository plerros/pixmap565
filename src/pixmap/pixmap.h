// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef PIXMAP565_PIXMAP_H
#define PIXMAP565_PIXMAP_H

#include <stdio.h>
#include <stdint.h>

struct pixmap;

void pixmap_new(struct pixmap **ptr, unsigned long x);
void pixmap_free(struct pixmap *ptr);

void pixmap_add(struct pixmap *ptr, uint16_t pixel);
int pixmap_read(struct pixmap *ptr, FILE *fp);
int pixmap_write(struct pixmap *ptr, FILE *fp);

#endif /* PIXMAP565_PIXMAP_H */
