// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "file_utils.h"
#include "llnode.h"

struct llnode
{
	struct llnode *next;
	uint16_t *array;
	udword_t size;
	udword_t logical_size; // first unoccupied element
};

void llnode_new(struct llnode **ptr, udword_t size)
{
	struct llnode *new = malloc(sizeof(struct llnode));
	if (new == NULL)
		abort();

	new->size = size;
	new->array = malloc(sizeof(udword_t) * new->size);
	if (new->array == NULL)
		abort();

	new->logical_size = 0;
	new->next = NULL;

	*ptr = new;
}

void llnode_free(struct llnode *ptr)
{
	if (ptr == NULL)
		return;
	do {
		struct llnode *next = ptr->next;

		free(ptr->array);
		free(ptr);
		ptr = next;
	} while (ptr != NULL);
}

struct llnode *llnode_add(struct llnode *ptr, uint16_t value)
{
	assert(ptr != NULL);
	while (ptr->next != NULL)
		ptr = ptr->next;

	if (ptr->logical_size >= ptr->size) {
		llnode_new(&(ptr->next), ptr->size);
		ptr = ptr->next;
	}
	ptr->array[ptr->logical_size] = value;
	ptr->logical_size += 1;
	return ptr;
}

void llnode_reverse_data(struct llnode *ptr)
{
	do {
		assert(ptr->logical_size == ptr->size);
		for (udword_t i = 0; i < ptr->logical_size / 2; i++) {
			uint16_t tmp = ptr->array[i];
			ptr->array[i] = ptr->array[ptr->logical_size -1 - i];
			ptr->array[ptr->logical_size -1 - i] = tmp;
		}
		ptr = ptr->next;
	} while (ptr != NULL);
}

void llnode_reverse_nodes(struct llnode *ptr)
{
	struct llnode *prev = NULL;
	do {
		struct llnode *next = ptr->next;

		ptr->next = prev;
		prev = ptr;
		ptr = next;
	} while (ptr != NULL);
}

bool llnode_is_full(struct llnode *ptr)
{
	assert(ptr->logical_size <= ptr->size);
	return (ptr->logical_size == ptr->size);
}

int llnode_write(struct llnode *ptr, FILE *fp)
{
	int rc = 0;
	do {
		assert(ptr->logical_size == ptr->size);
		for (udword_t i = 0; i < ptr->logical_size; i++) {
			rc = fput_uword(ptr->array[i], fp);
			if (rc)
				goto out;
		}
		for (udword_t i = 0; i < (ptr->logical_size * BYTES_PER_PIXEL) % 4; i++) {
			rc = (fputc(0, fp) != 0);
			if (rc)
				goto out;
		}
		ptr = ptr->next;
	} while (ptr != NULL);

out:
	return rc;
}
