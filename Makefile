CC=gcc
CFLAGS=
CXX=g++
CxXFLAGS=
MAKE=make
TARGET=osecpu
SOURCE=main.c osecpu.c api.c window.c
SOURCE_CXX=debugger.cpp
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
	$(RM) $(TARGET)-cxx.o

$(TARGET)-cxx.o: $(SOURCE_CXX)
	$(CXX) -o $(TARGET)-cxx.o -c `pkg-config --cflags --libs gtkmm-2.4` $(CXXFLAGS) $(SOURCE_CXX)

$(TARGET): $(SOURCE) $(TARGET)-cxx.o
	$(CC) -o $(TARGET) `pkg-config --cflags --libs gtk+-2.0` $(CFLAGS) $(SOURCE) $(TARGET)-cxx.o

