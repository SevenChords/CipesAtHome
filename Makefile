CFLAGS=-lcurl -lconfig -fopenmp -Wall -O2
TARGET=recipesAtHome
DEPS=start.h inventory.h recipes.h config.h FTPManagement.h cJSON.h calculator.h logger.h
OBJ=start.o inventory.o recipes.o config.o FTPManagement.o cJSON.o calculator.o logger.o

UNAME:=$(shell uname)
ifeq ($(UNAME), Linux)
	CC=gcc
endif
ifeq ($(UNAME), Darwin)
	MACPREFIX:=$(shell brew --prefix)
	CC:=$(MACPREFIX)/opt/llvm/bin/clang
	CFLAGS:=-I$(MACPREFIX)/include -L$(MACPREFIX)/lib $(CFLAGS)
endif

default: $(TARGET)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	$(RM) ./*.o
	$(RM) ./$(TARGET)
