CC=gcc
CFLAGS=
MAKE=make
RM=rm -f

.PHONY: default test clean
default:
	@$(MAKE) --no-print-directory test

test:
	$(CC) -shared -fPIC -o test.so test.c ../osecpu.c `pkg-config --cflags --libs cutter`
	cutter .

clean:
	$(RM) test.so

