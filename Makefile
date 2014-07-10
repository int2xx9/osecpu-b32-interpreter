CC=gcc
CFLAGS=
MAKE=make --no-print-directory
TARGET=osecpu
SOURCE=main.c
RM=rm -f

.PHONY: default all clean
default:
	@$(MAKE) all

all:
	@$(MAKE) osecpu

clean:
	$(RM) $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(CFLAGS) $(SOURCE)

