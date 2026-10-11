CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = bin/ls
SRC = src/ls-v1.0.0.c

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -rf bin/ls obj/*.o

.PHONY: all clean

