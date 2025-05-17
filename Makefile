SRC_DIR := src
BIN_DIR := bin

SRC_FILE := $(SRC_DIR)/cip.c
BIN_FILE := $(BIN_DIR)/cip

CC := gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS := -lcurl

.PHONY: all clean install

all: $(BIN_FILE)

$(BIN_FILE): $(SRC_FILE) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(SRC_FILE) -o $(BIN_FILE) $(LDFLAGS)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf $(BIN_DIR)

# requires sudo
install: $(BIN_FILE)
	sudo cp $(BIN_FILE) /usr/local/bin/cip
