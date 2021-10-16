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

//	FILE *out = fopen("out.bmp", "w+");
//	assert(out != NULL);

	// (test code)
	struct picture *pic = NULL;
	picture_new(&pic);
	picture_read(pic, fp);
//	picture_write(pic, out);
	picture_free(pic);

//	fclose(out);
	fclose(fp);
	return 0;
}
