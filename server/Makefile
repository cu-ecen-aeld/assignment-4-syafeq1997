# Variables
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g -Wno-error=unused-variable
TARGET ?= aesdsocket
SRC ?= aesdsocket.c
OBJ ?= $(SRC:.c=.o)
LDFLAGS ?= -lpthread -lrt

all: $(TARGET)

default: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)

