CC := gcc

CFLAGS := -fsanitize=address -Wall -Wextra --std=c17 -I./src/include

SRC := $(shell find src -name "*.c")
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))

TARGET := bin/out

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CC) -fsanitize=address $(OBJ) -o $(TARGET)

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf obj bin

.PHONY: all run clean
