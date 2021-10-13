// SPDX-License-Identifier: GPL-3.0-or-later
/*
 * Copyright (c) 2021 Pierro Zachareas
 */

#ifndef PIXMAP565_LLNODE_H
#define PIXMAP565_LLNODE_H

#include <stdint.h>

struct llnode;

void llnode_new(struct llnode **ptr, unsigned long size);
void llnode_free(struct llnode *ptr);

struct llnode *llnode_add(struct llnode *ptr, uint16_t value);

#endif /* PIXMAP565_LLNODE_H */
