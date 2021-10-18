// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "file_utils.h"

void print_error()
{
	fprintf(stderr, "\n[ERROR]: ");
}

udword_t dword_abs(dword_t value)
{
	if (value < 0)
		value *= -1;
	
	return value;
}

int fput_any_word(unsigned long long *value, size_t size, FILE *fp)
{
	int rc = 0;
	for (size_t i = 0; i < size && !rc; i++) {
		int tmp = *value % (UCHAR_MAX + 1); // & 0x0f would probably be just fine
		*value = *value / (UCHAR_MAX + 1);
		rc = (fputc(tmp, fp) != tmp);
	}
	return rc;
}

int fput_uword(uword_t value, FILE *fp)
{
	int rc = 0;
	unsigned long long tmp = value;
	rc = fput_any_word(&tmp, 2, fp);
	return rc;
}

int fput_dword(dword_t value, FILE *fp)
{
	int rc = 0;
	unsigned long long tmp = dword_abs(value);

	rc = fput_any_word(&tmp, 3, fp);
	if (rc)
		goto out;

	assert(tmp <= (SCHAR_MAX + 1));
	unsigned char uch = tmp;
	signed char *sch = (signed char *)(&uch);
	
	if (value < 0)
		*sch *= -1;

	rc = (fputc(uch, fp) != uch);
out:
	return rc;
}

int fput_udword(uword_t value, FILE *fp)
{
	int rc = 0;
	unsigned long long tmp = value;
	rc = fput_any_word(&tmp, 4, fp);
	return rc;
}