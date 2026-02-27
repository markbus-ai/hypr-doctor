CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
SRC = main.c src/config_parser.c src/env_check.c src/gpu_check.c src/log_check.c src/proc_check.c
OBJ = $(SRC:.c=.o)
TARGET = hypr-doctor

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f src/*.o *.o $(TARGET)

.PHONY: all clean
