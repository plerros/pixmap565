// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#include "file_utils.h"
#include "pixmap.h"
#include "llnode.h"

struct pixmap
{
	udword_t resx;
	udword_t resy;
	struct llnode *first;
	struct llnode *last;
};

void pixmap_new(struct pixmap **ptr, udword_t x)
{
	assert(ptr != NULL);
	assert(*ptr == NULL);

	struct pixmap *new = malloc(sizeof(struct pixmap));
	if (new == NULL)
		abort();

	new->resx = x;
	new->resy = 0;
	new->first = NULL;
	new->last = NULL;

	*ptr = new;
}

void pixmap_free(struct pixmap *ptr)
{
	if (ptr != NULL)
		llnode_free(ptr->first);

	free(ptr);
}

void pixmap_flip_x(struct pixmap *ptr)
{
	assert(ptr != NULL);
	llnode_reverse_data(ptr->first);
}

void pixmap_flip_y(struct pixmap *ptr)
{
	assert(ptr != NULL);
	llnode_reverse_nodes(ptr->first);
	struct llnode *tmp = ptr->first;
	ptr->first = ptr->last;
	ptr->last = tmp;
}

void pixmap_add(struct pixmap *ptr, uint16_t pixel)
{
	assert(ptr != NULL);
	if (ptr->first == NULL) {
		llnode_new(&(ptr->first), ptr->resx);
		ptr->last = ptr->first;
		ptr->resy += 1;
	}
	struct llnode *tmp = ptr->last;
	ptr->last = llnode_add(ptr->last, pixel);
	if (tmp != ptr->last)
		ptr->resy += 1;
}

udword_t pixmap_get_x(struct pixmap *ptr)
{
	return(ptr->resx);
}

udword_t pixmap_get_y(struct pixmap *ptr)
{
	return(ptr->resy);
}

int pixmap_read(struct pixmap *ptr, FILE *fp)
{
	assert(ptr != NULL);
	int rc = 0;

	enum item_ids{
		pixel_line, // pixel(s)
		padding     // padding
	};
	enum item_types {
		pixel, // width*height*bytesperpixel
		skip   // skip_bytes
	};
	int type[] = {
		pixel,
		skip
	};

	udword_t offset = 0; // byte offset of the current item
	udword_t byte = 0;   // # of bytes from the file start

	uword_t uw_value = 0;
	int item = 0;
	int skip_bytes = 0;

	while (rc != 1) {
		int ch = fgetc(fp);

		if (feof(fp)) {
			if (!llnode_is_full(ptr->last)) {
				print_error();
				fprintf(stderr, "Unexpected end of file.\n");
				rc = 1;
			}
			goto out;
		}
		if (ferror(fp)) {
			print_error();
			fprintf(stderr, "Unexpected end of file, caused by I/O error.\n");
			rc = 1;
			goto out;
		}
		if (ch > UCHAR_MAX) {
			print_error();
			fprintf(stderr, "The value of byte %llu is out of range [0, %u].\n", (unsigned long long)byte, UCHAR_MAX);
			rc = 1;
			goto out;
		}

		udword_t item_size = 0;
		switch (type[item]) {
		case skip:
			item_size = skip_bytes;
			break;

		case pixel:
			uw_value += ((uword_t)ch) << ((byte - offset) % BYTES_PER_PIXEL) * CHAR_BIT;
			if ((byte - offset) % BYTES_PER_PIXEL == BYTES_PER_PIXEL - 1) {
				pixmap_add(ptr, uw_value);
				uw_value = 0;
			}
			assert(ptr->resx != 0);
			item_size = ptr->resx * BYTES_PER_PIXEL;
			break;
		}
		byte++;

		if (byte - offset == item_size) {
			skip_bytes = 0;
			switch (item) {
			case pixel_line:
				if ((ptr->resx * BYTES_PER_PIXEL) % 4 != 0) {
					skip_bytes = 4 - ((ptr->resx * BYTES_PER_PIXEL) % 4);
					item = padding;
				}
				break;

			case padding:
				item = pixel_line;
				break;
			}
			uw_value = 0;
			offset = byte;
		}
	}

out:
	return rc;
}

int pixmap_write(struct pixmap *ptr, FILE *fp)
{
	assert(ptr != NULL);
	int rc = 0;
	rc = llnode_write(ptr->first, fp);
	return rc;
}
