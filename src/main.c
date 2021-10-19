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
	// (test code)
	char in[] = "about.bmp";
	char out[] = "out.bin";

	struct picture *pic = NULL;
	struct pixmap *pix = NULL;

	FILE *infile = fopen(in, "r");
	assert(infile != NULL);

	FILE *outfile = fopen(out, "w+");
	assert(outfile != NULL);

	if (is_pic(in))
	{
		picture_new(&pic);
		picture_read(pic, infile);
		pix = picture_get_pixmap(pic);
		pixmap_write(pix, outfile);
	} else {
		pixmap_new(&pix, 78);
		pixmap_read(pix, infile);
		picture_set_pixmap(pic, pix);
		pix = NULL;
		picture_write(pic, outfile);
	}
	picture_free(pic);
	pixmap_free(pix);

	fclose(infile);
	fclose(outfile);
	return 0;
}
