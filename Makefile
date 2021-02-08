WARNINGS_AND_ERRORS?=-Wall -Werror=format-overflow -Werror=format-truncation -Werror=format-extra-args -Werror=format -Werror=maybe-uninitialized -Werror=array-bounds -Werror=narrowing -Werror=unknown-pragmas -Werror=multichar
WARNINGS_AND_ERRORS_CC?=-Werror=implicit-function-declaration -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=discarded-qualifiers
WARNINGS_AND_ERRORS_CXX?=
BASE_CFLAGS=-lcurl -lconfig -fopenmp -I . -Wall -O2 $(WARNINGS_AND_ERRORS) $(WARNINGS_AND_ERRORS_CC)
USER_CFLAGS:=$(CFLAGS)
CFLAGS=$(BASE_CFLAGS) $(USER_CFLAGS)
FINAL_TARGET_CFLAGS?=-Wl,--gc-sections
TARGET=recipesAtHome
DEPS=start.h inventory.h recipes.h config.h FTPManagement.h cJSON.h calculator.h logger.h shutdown.h base.h $(wildcard absl/base/*.h )
OBJ=start.o inventory.o recipes.o config.o FTPManagement.o cJSON.o calculator.o logger.o shutdown.o

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
	$(CC) -o $@ $^ $(CFLAGS) $(FINAL_TARGET_CFLAGS)

all: $(TARGET)

.PHONY: all clean

clean:
	$(RM) ./*.o
	$(RM) ./$(TARGET)
