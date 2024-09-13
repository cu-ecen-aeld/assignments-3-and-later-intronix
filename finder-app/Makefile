CC := $(CROSS_COMPILE)gcc
CFLAGS := -Wall -Wextra -O2
LDFLAGS :=

TARGET := writer

all: $(TARGET)

$(TARGET): writer.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) writer.c

clean:
	rm -f $(TARGET)

.PHONY: all clean
