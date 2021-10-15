// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>

#include "picture.h"
#include "pixmap.h"

int main(int argc, char *argv[])
{
	FILE *fp = fopen("about.bmp", "r");
	assert(fp != NULL);

	// (test code)
	struct picture *pic = NULL;
	picture_new(&pic);
//	picture_write(pic, fp);
	picture_read(pic, fp);
	picture_free(pic);

	fclose(fp);
	return 0;
}
