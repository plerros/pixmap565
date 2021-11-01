// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef PIXMAP565_LLNODE_H
#define PIXMAP565_LLNODE_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * llnode:
 *
 * An unrolled linked list node
 */

struct llnode;

void llnode_new(struct llnode **ptr, udword_t size);
void llnode_free(struct llnode *ptr);

struct llnode *llnode_add(struct llnode *ptr, uint16_t value);

void llnode_reverse_data(struct llnode *ptr);
void llnode_reverse_nodes(struct llnode *ptr);

bool llnode_is_full(struct llnode *ptr);
int llnode_write(struct llnode *ptr, FILE *fp);

#endif /* PIXMAP565_LLNODE_H */
