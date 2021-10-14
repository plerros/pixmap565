// SPDX-License-Identifier: MIT
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef PIXMAP565_PICTURE_H
#define PIXMAP565_PICTURE_H

#include <stdio.h>

struct picture;

void picture_new(struct picture **ptr);
void picture_free(struct picture *ptr);

void picture_read(struct picture *ptr, FILE *fp);
void picture_write(struct picture *ptr, FILE *fp);

#endif /* PIXMAP565_PICTURE_H */
