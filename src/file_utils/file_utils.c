// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <stdio.h>

#include "file_utils.h"

void print_error(void)
{
	fprintf(stderr, "\n[ERROR]: ");
}

void print_warning(void)
{
	fprintf(stderr, "\n[WARNING]: ");
}

udword_t dword_abs(dword_t value)
{
	if (value < 0)
		value *= -1;

	return value;
}

static int fput_any_word(udword_t value, size_t size, FILE *fp)
{
	int rc = 0;
	for (size_t i = 0; i < size && !rc; i++) {
		int tmp = value % (UCHAR_MAX + 1); // & 0x0f would probably be just fine
		value >>= CHAR_BIT;
		rc = (fputc(tmp, fp) != tmp);
	}
	return rc;
}

int fput_uword(uword_t value, FILE *fp)
{
	return (fput_any_word(value, 2, fp));
}

int fput_dword(dword_t value, FILE *fp)
{
	udword_t *u_value = (void *)&value;
	return (fput_any_word(*u_value, 4, fp));
}

int fput_udword(udword_t value, FILE *fp)
{
	return (fput_any_word(value, 4, fp));
}
