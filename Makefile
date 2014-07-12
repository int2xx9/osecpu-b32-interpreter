CC=gcc
CFLAGS=
MAKE=make
TARGET=osecpu
SOURCE=main.c osecpu.c
RM=rm -f

.PHONY: default all test clean
default:
	@$(MAKE) --no-print-directory all

all:
	@$(MAKE) --no-print-directory $(TARGET)

test:
	@$(MAKE) -C tests

clean:
	@$(MAKE) -C tests clean
	$(RM) $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(CFLAGS) $(SOURCE)

