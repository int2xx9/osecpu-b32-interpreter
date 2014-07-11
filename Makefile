CC=gcc
CFLAGS=
MAKE=make --no-print-directory
TARGET=osecpu
SOURCE=osecpu.c
RM=rm -f

.PHONY: default all clean
default:
	@$(MAKE) all

all:
	@$(MAKE) $(TARGET)

clean:
	$(RM) $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) -o $(TARGET) $(CFLAGS) $(SOURCE)

