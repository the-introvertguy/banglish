CC      := gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS := -lm
TARGET  := bin/banglish
SRC     := $(wildcard src/*.c)
OBJ     := $(patsubst src/%.c,build/%.o,$(SRC))

.PHONY: all clean rebuild run

all: $(TARGET)

$(TARGET): $(OBJ) | bin
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build bin:
	mkdir -p $@

run: $(TARGET)
	./$(TARGET) examples/source.bs

rebuild: clean all

clean:
	rm -rf build bin
