# Makefile - builds the demo under several preprocessor presets.
#
#   make all / make run   default trace build (dev)
#   make release           logging compiled out entirely, CONFIG_DEBUG=0
#   make errors-only       only WARN/ERROR/FATAL survive compilation
#   make clean
#
# Override the compiler with  make CC=clang   (mingw32-make works too).
# On Windows the produced binaries get a .exe suffix and clean uses del.

# Use 'gcc' unconditionally (a ?= would not override make's built-in
# default CC=cc). You can still choose another compiler via:
#   make CC=clang all
CC      := gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -O2
LDLIBS  ?=

SRC     = src/log.c src/demo.c

ifeq ($(OS),Windows_NT)
EXE   := .exe
RM    := del /q
else
EXE   :=
RM    := rm -f
endif

# --- default (dev) profile -------------------------------------------------
DEFS_DEV = -DCONFIG_LOG_ENABLE=1 \
           -DCONFIG_LOG_LEVEL=LOG_LEVEL_TRACE \
           -DCONFIG_DEBUG=1 \
           -DCONFIG_LOG_TIME=1 \
           -DCONFIG_LOG_COLOR=1 \
           -DCONFIG_LOG_FILENAME=1

# --- variants --------------------------------------------------------------
DEFS_RELEASE    = -DCONFIG_LOG_ENABLE=0
DEFS_ERRORS     = -DCONFIG_LOG_ENABLE=1 \
                  -DCONFIG_LOG_LEVEL=LOG_LEVEL_WARN \
                  -DCONFIG_DEBUG=0

.PHONY: all release errors-only run clean

all: demo$(EXE)

demo$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(DEFS_DEV) -Iinclude -o $@ $(SRC) $(LDLIBS)

release: release$(EXE)

release$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(DEFS_RELEASE) -Iinclude -o $@ $(SRC) $(LDLIBS)

errors-only: errors$(EXE)

errors$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(DEFS_ERRORS) -Iinclude -o $@ $(SRC) $(LDLIBS)

run: demo$(EXE)
	./demo$(EXE)

clean:
	-$(RM) demo$(EXE) release$(EXE) errors$(EXE) *.o