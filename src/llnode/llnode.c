// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>

#include "llnode.h"

struct llnode
{
	struct llnode *next;
	struct llnode *prev;
	uint16_t *array;
	unsigned long size;
	unsigned long logical_size; // first unoccupied element
};

void llnode_new(struct llnode **ptr, unsigned long size)
{
	struct llnode *new = malloc(sizeof(struct llnode));
	if (new == NULL)
		abort();

	new->size = size;
	new->array = malloc(sizeof(unsigned long) * new->size);
	if (new->array == NULL)
		abort();

	new->logical_size = 0;
	new->next = NULL;
	new->prev = NULL;

	*ptr = new;
}

void llnode_free(struct llnode *ptr)
{
	if (ptr == NULL)
		return;

	while (ptr->prev != NULL) {
		assert(ptr == ptr->prev->next);
		ptr = ptr->prev;
	}
	do {
		struct llnode *next = ptr->next;
		if (next != NULL)
			assert(ptr == next->prev);

		free(ptr->array);
		free(ptr);
		ptr = next;
	} while (ptr != NULL);
}

struct llnode *llnode_add(struct llnode *ptr, uint16_t value)
{
	assert(ptr != NULL);
	assert(ptr->next == NULL);

	while (ptr->next != NULL) {
		ptr = ptr->next;
	}
	if (ptr->logical_size >= ptr->size) {
		llnode_new(&(ptr->next), ptr->size);
		ptr->next->prev = ptr;
		ptr = ptr->next;
	}
	ptr->array[ptr->logical_size] = value;
	ptr->logical_size += 1;
	return ptr;
}

void llnode_flip_x(struct llnode *ptr)
{
	assert(ptr != NULL);
	while (ptr->prev != NULL) {
		assert(ptr == ptr->prev->next);
		ptr = ptr->prev;
	}
	do {
		assert(ptr->logical_size == ptr->size);
		for (unsigned long i = 0; i < ptr->logical_size / 2; i++) {
			uint16_t tmp = ptr->array[i];
			ptr->array[i] = ptr->array[ptr->logical_size -1 - i];
			ptr->array[ptr->logical_size -1 - i] = tmp;
		}
	} while (ptr != NULL);
}

void llnode_flip_y(struct llnode *ptr)
{
	assert(ptr != NULL);
	while (ptr->prev != NULL) {
		assert(ptr == ptr->prev->next);
		ptr = ptr->prev;
	}
	do {
		struct llnode *next = ptr->next;

		ptr->next = ptr->prev;
		ptr->prev = next; 

		ptr = next;
	} while (ptr != NULL);
}

int llnode_write(struct llnode *ptr, FILE *fp)
{
	assert(ptr != NULL);
	int rc = 0;
	while (ptr->prev != NULL) {
		assert(ptr == ptr->prev->next);
		ptr = ptr->prev;
	}
	do {
		assert(ptr->logical_size == ptr->size);
		for (unsigned long i = 0; i < ptr->logical_size; i++) {
			uint16_t value = ptr->array[i];
			for (int j = 0; j < 2; j++) {
				int tmp = value % (UCHAR_MAX + 1); // & 0x0f would probably be just fine
				value = value / (UCHAR_MAX + 1);
				rc = (fputc(tmp, fp) != tmp);
				if (rc)
					goto out;
			}
		}
		ptr = ptr->next;
	} while (ptr != NULL);

out:
	return rc;
}