WARNINGS_AND_ERRORS?=-Wall -Werror=format-overflow -Werror=format-truncation -Werror=format-extra-args -Werror=format -Werror=maybe-uninitialized -Werror=array-bounds -Werror=narrowing -Werror=multichar
WARNINGS_AND_ERRORS_CC?=-Werror=implicit-function-declaration -Werror=implicit-int -Werror=incompatible-pointer-types -Werror=discarded-qualifiers
WARNINGS_AND_ERRORS_CXX?=
BASE_CFLAGS=-lcurl -lconfig -fopenmp -I . -Wall -O2 $(WARNINGS_AND_ERRORS) $(WARNINGS_AND_ERRORS_CC)
USER_CFLAGS:=$(CFLAGS)
CFLAGS=$(BASE_CFLAGS) $(USER_CFLAGS)
FINAL_TARGET_CFLAGS?=-Wl,--gc-sections
TARGET=recipesAtHome
DEPS=start.h inventory.h recipes.h config.h FTPManagement.h cJSON.h node_print.h calculator.h logger.h shutdown.h base.h pcg_basic.h $(wildcard absl/base/*.h )
OBJ=start.o inventory.o recipes.o config.o FTPManagement.o cJSON.o node_print.o calculator.o logger.o shutdown.o pcg_basic.o

UNAME:=$(shell uname)
ifeq ($(UNAME), Linux)
	CC=gcc
endif
ifeq ($(UNAME), Darwin)
	MACPREFIX:=$(shell brew --prefix)
	CC:=$(MACPREFIX)/opt/llvm/bin/clang
	CFLAGS:=-I$(MACPREFIX)/include -L$(MACPREFIX)/lib $(CFLAGS)
	FINAL_TARGET_CFLAGS:=$(subst --gc-sections,-dead_strip,$(FINAL_TARGET_CFLAGS))
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
