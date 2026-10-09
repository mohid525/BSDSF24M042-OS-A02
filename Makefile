CC = gcc
CFLAGS = -Wall -Wextra -std=gnu11

TARGET = bin/ls
SOURCE = src/lsv1.0.0.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	mkdir -p bin
	$(CC) $(CFLAGS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET)
