// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef PIXMAP565_PICTURE_H
#define PIXMAP565_PICTURE_H

#define PICTURE_EXTENSION ".bmp"
#define PICTURE_TYPE "BMP565"

#include <stdio.h>

#include "pixmap.h"

struct picture;

void picture_new(struct picture **ptr);
void picture_free(struct picture *ptr);

void picture_set_pixmap(struct picture *ptr, struct pixmap *matrix);
struct pixmap *picture_get_pixmap(struct picture *ptr);

bool is_pic(char *filename);

int picture_read(struct picture *ptr, FILE *fp);
int picture_write(struct picture *ptr, FILE *fp);

#endif /* PIXMAP565_PICTURE_H */
