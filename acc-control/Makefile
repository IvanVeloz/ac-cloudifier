CC := gcc

CFLAGS := -Wall -Wextra -pthread -g -I./include
LDFLAGS += -lpigpiod_if2 -lrt -pthread

SRC_DIR := source
OBJ_DIR := obj
OUT_DIR := bin

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS))
OUT := $(OUT_DIR)/acc-control

all: $(OUT)

# Rule to compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to link object files into the executable
$(OUT): $(OBJS) | $(OUT_DIR)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@ 

# Create directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OUT_DIR):
	mkdir -p $(OUT_DIR)

# Clean rule
clean:
	rm -rf $(OBJ_DIR) $(OUT_DIR)

.PHONY: all clean
