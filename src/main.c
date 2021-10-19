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
	int rc = 0;
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
		rc = picture_read(pic, infile);
		if (rc)
			goto out;
		pix = picture_get_pixmap(pic);
		rc = pixmap_write(pix, outfile);
		if (rc)
			goto out;
	} else {
		pixmap_new(&pix, 78);
		rc = pixmap_read(pix, infile);
		if (rc)
			goto out;
		picture_set_pixmap(pic, pix);
		pix = NULL;
		rc = picture_write(pic, outfile);
		if (rc)
			goto out;
	}

out:
	picture_free(pic);
	pixmap_free(pix);

	fclose(infile);
	fclose(outfile);
	return rc;
}
