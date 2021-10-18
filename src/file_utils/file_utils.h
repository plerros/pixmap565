// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef PIXMAP565_FILE_UTILS_H
#define PIXMAP565_FILE_UTILS_H

#define BYTES_PER_PIXEL (8 / CHAR_BIT + 1)

#if (CHAR_BIT % 8) != 0
#error unsupported architecture
#endif

#include <stdio.h>

typedef unsigned short uword_t;
typedef long dword_t;
typedef unsigned long udword_t;

void print_error();

udword_t dword_abs(dword_t value);

int fput_uword(uword_t value, FILE *fp);
int fput_dword(dword_t value, FILE *fp);
int fput_udword(uword_t value, FILE *fp);

#endif /* PIXMAP565_FILE_UTILS_H */
