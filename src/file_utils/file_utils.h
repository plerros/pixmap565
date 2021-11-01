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

// Pick an integer type that can hold 2 bytes
#if (USHRT_MAX >> CHAR_BIT >= UCHAR_MAX)
	typedef unsigned short uword_t;
#elif (UINT_MAX >> CHAR_BIT >= UCHAR_MAX)
	typedef unsigned int uword_t;
#elif (ULONG_MAX >> CHAR_BIT >= UCHAR_MAX)
	typedef unsigned long uword_t;
#elif (ULLONG_MAX >> CHAR_BIT >= UCHAR_MAX)
	typedef unsigned long long uword_t;
#else
#error Unsupported system: None of the integer types can hold 2 bytes.
#endif

// Pick an integer type with value bits equal to 4 bytes
#if (USHRT_MAX >> 3 * CHAR_BIT == UCHAR_MAX)
#define UDWORD_MAX USHRT_MAX
	typedef short dword_t;
	typedef unsigned short udword_t;
#elif (UINT_MAX >> 3 * CHAR_BIT == UCHAR_MAX)
#define UDWORD_MAX UINT_MAX
	typedef int dword_t;
	typedef unsigned int udword_t;
#elif (ULONG_MAX >> 3 * CHAR_BIT == UCHAR_MAX)
#define UDWORD_MAX ULONG_MAX
	typedef long dword_t;
	typedef unsigned long udword_t;
#elif (ULLONG_MAX >> 3 * CHAR_BIT == UCHAR_MAX)
#define UDWORD_MAX ULLONG_MAX
	typedef long long dword_t;
	typedef unsigned long long udword_t;
#else
#error Unsupported system: None of the integer types have value bits equal to 4 bytes.
#endif

void print_error(void);
void print_warning(void);

udword_t dword_abs(dword_t value);

int fput_uword(uword_t value, FILE *fp);
int fput_dword(dword_t value, FILE *fp);
int fput_udword(udword_t value, FILE *fp);

#endif /* PIXMAP565_FILE_UTILS_H */
