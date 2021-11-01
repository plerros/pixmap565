# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (c) 2021 Pierro Zachareas

TARGET = pixmap565
BUILD = ./build

CC := $(shell \
	for compiler in cc gcc clang; \
	do \
		if command -v $$compiler; then \
			break; \
		fi \
	done \
)

WARNINGS = -Wall -Wextra
OPTIMIZE = -O2

all: builddir $(TARGET)
builddir:
	mkdir -p $(BUILD)

$(TARGET): $(BUILD)/file_utils.o $(BUILD)/llnode.o $(BUILD)/main.o $(BUILD)/picture.o $(BUILD)/pixmap.o
	$(CC) $(WARNINGS) $(OPTIMIZE) $^ -o $@

$(BUILD)/file_utils.o: ./src/file_utils/file_utils.c
	$(CC) $(WARNINGS) $(OPTIMIZE) -c $^ -o $@

$(BUILD)/llnode.o: ./src/llnode/llnode.c
	$(CC) $(WARNINGS) $(OPTIMIZE) -I ./src/file_utils -c $^ -o $@

$(BUILD)/main.o: ./src/main.c
	$(CC) $(WARNINGS) $(OPTIMIZE) -I ./src/file_utils -I ./src/picture -I ./src/pixmap -c $^ -o $@

$(BUILD)/picture.o: ./src/picture/picture.c
	$(CC) $(WARNINGS) $(OPTIMIZE) -I ./src/file_utils -I ./src/pixmap -c $^ -o $@

$(BUILD)/pixmap.o: ./src/pixmap/pixmap.c
	$(CC) $(WARNINGS) $(OPTIMIZE) -I ./src/file_utils -I ./src/llnode -c $^ -o $@

.PHONY:
clean:
	rm -f $(TARGET)
	rm -rf $(BUILD)
