# Variables
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g
TARGET = writer
SRC = writer.c
OBJ = $(SRC:.c=.o)

default: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)

