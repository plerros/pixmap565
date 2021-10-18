# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Pierro Zachareas

TARGET = pixmap565
SRC_DIR = ./src
BUILD_DIR = ./build

CC := $(shell \
	for compiler in cc gcc clang; \
	do \
		if command -v $$compiler; then \
			break; \
		fi \
	done \
)

$(TARGET): builddir llnode.o main.o picture.o pixmap.o
	$(CC) $(BUILD_DIR)/llnode.o $(BUILD_DIR)/main.o $(BUILD_DIR)/picture.o $(BUILD_DIR)/pixmap.o -o $@

builddir:
	mkdir -p $(BUILD_DIR)

llnode.o: $(SRC_DIR)/llnode/llnode.c
	$(CC) -c $< -o $(BUILD_DIR)/llnode.o

main.o: $(SRC_DIR)/main.c
	$(CC) -I $(SRC_DIR)/picture -I $(SRC_DIR)/pixmap -c $< -o $(BUILD_DIR)/main.o

picture.o: $(SRC_DIR)/picture/picture.c
	$(CC) -I $(SRC_DIR)/pixmap -c $< -o $(BUILD_DIR)/picture.o

pixmap.o: $(SRC_DIR)/pixmap/pixmap.c
	$(CC) -I $(SRC_DIR)/llnode -c $< -o $(BUILD_DIR)/pixmap.o

.PHONY:
clean:
	rm -f $(TARGET)
	rm -rf $(BUILD_DIR)
