// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef PIXMAP565_PIXMAP_H
#define PIXMAP565_PIXMAP_H

#include <stdint.h>
#include <stdio.h>

struct pixmap;

void pixmap_new(struct pixmap **ptr, unsigned long x);
void pixmap_free(struct pixmap *ptr);

void pixmap_flip_x(struct pixmap *ptr);
void pixmap_flip_y(struct pixmap *ptr);
void pixmap_add(struct pixmap *ptr, uint16_t pixel);
unsigned long pixmap_get_x(struct pixmap *ptr);
unsigned long pixmap_get_y(struct pixmap *ptr);
int pixmap_read(struct pixmap *ptr, FILE *fp);
int pixmap_write(struct pixmap *ptr, FILE *fp);

#endif /* PIXMAP565_PIXMAP_H */
