// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <stdbool.h>

#include "picture.h"
#include "pixmap.h"

int main(int argc, char *argv[])
{
	bool flip_x = false;
	bool flip_y = false;

	FILE *fp = fopen("about.bmp", "r");
	assert(fp != NULL);

	FILE *out = fopen("out.bmp", "w+");
	assert(out != NULL);

	// (test code)
	struct picture *pic = NULL;
	picture_new(&pic);

	struct pixmap *pix = NULL;

	picture_read(pic, fp);

	if (flip_x)
		pixmap_flip_x(pix);

	if (flip_y)
		pixmap_flip_y(pix);

	picture_write(pic, out);
	picture_free(pic);

	fclose(out);
	fclose(fp);
	return 0;
}
