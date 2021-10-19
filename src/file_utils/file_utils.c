// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <stdlib.h>
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
		*value = *value >> CHAR_BIT;
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

	short      ss = value;
	int        si = value;
	long       sl = value;
	long long sll = value;

	unsigned char *arr = NULL;
	if (sizeof(short) == 4) {
		arr = (void *)(&ss);
	}
	else if (sizeof(int) == 4) {
		arr = (void *)(&si);
	}
	else if (sizeof(long) == 4) {
		arr = (void *)(&sl);
	}
	else if (sizeof(long long) == 4) {
		arr = (void *)(&sll);
	}
	else {
		fprintf(stderr, "Unsupported system: None of the integer types are 4 bytes long\n");
		abort();
	}
	unsigned long long pval = 0;
	for (int i = 3; i >= 0; i--)
		pval = (pval << CHAR_BIT) + arr[i];

	rc = fput_any_word(&pval, 4, fp);
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