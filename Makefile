CC=gcc
CFLAGS=
MAKE=make --no-print-directory
TARGET=osecpu
SOURCE=main.c osecpu.c
RM=rm -f

.PHONY: default all test clean
default:
	@$(MAKE) all

all:
	@$(MAKE) $(TARGET)

test:
	@$(MAKE) -C tests

clean:
	$(RM) $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(CFLAGS) $(SOURCE)

