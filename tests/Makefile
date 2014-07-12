CC=gcc
CFLAGS=
MAKE=make
RM=rm -f
TARGET=test.so
SOURCE=test.c ../osecpu.c

.PHONY: default test clean
default:
	@$(MAKE) --no-print-directory test

test:
	@$(MAKE) --no-print-directory $(TARGET)
	cutter .

clean:
	$(RM) test.so

$(TARGET): $(SOURCE)
	$(CC) -shared -fPIC -o $(TARGET) $(SOURCE) `pkg-config --cflags --libs cutter`

