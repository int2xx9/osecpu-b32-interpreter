CC=gcc
CFLAGS=
MAKE=make --no-print-directory
RM=rm -f

.PHONY: default test clean
default:
	@$(MAKE) test

test:
	$(CC) -shared -fPIC -o test.so test.c ../osecpu.c `pkg-config --cflags --libs cutter`
	cutter .

clean:
	$(RM) test.so

