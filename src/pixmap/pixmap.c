// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "pixmap.h"
#include "llnode.h"


struct pixmap
{
	unsigned long resx;
	unsigned long resy;
	bool flip_x;
	bool flip_y;
	struct llnode *first;
	struct llnode *last;
};

void pixmap_new(struct pixmap **ptr, unsigned long x)
{
	assert(ptr != NULL);
	assert(*ptr == NULL);

	struct pixmap *new = malloc(sizeof(struct pixmap));
	if (new == NULL)
		abort();

	new->resx = x;
	new->resy = 0;
	new->flip_x = false;
	new->flip_y = false;
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
	ptr->flip_x = true;
}

void pixmap_flip_y(struct pixmap *ptr)
{
	ptr->flip_y = true;
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

unsigned long pixmap_get_x(struct pixmap *ptr)
{
	return(ptr->resx);
}

unsigned long pixmap_get_y(struct pixmap *ptr)
{
	return(ptr->resy);
}

int pixmap_read(struct pixmap *ptr, FILE *fp)
{

}

int pixmap_write(struct pixmap *ptr, FILE *fp)
{
	assert(ptr != NULL);
	int rc = 0;
	rc = llnode_write(ptr->first, fp);
	return rc;
}
