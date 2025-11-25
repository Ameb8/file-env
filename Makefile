CC      = gcc
CFLAGS  = -Wall -Wextra -O2
SRC_DIR = src
INC_DIR = include
OBJ_DIR = build/obj
BIN_DIR = build/bin

TARGET  = $(BIN_DIR)/xfile

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC))

all: $(TARGET)

# Final binary
$(TARGET): $(OBJ) | $(BIN_DIR)
	$(CC) $(OBJ) -o $@

# Object file rule
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -I $(INC_DIR) -c $< -o $@

# Directory creation rules (order-only prerequisites)
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

clean:
	rm -rf build

.PHONY: all clean
