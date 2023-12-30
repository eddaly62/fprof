# Makefile for fprof library

.PHONY: all release install clean

# Library Version
SVER?=1.0
RVER?=$(SVER).0

OTHER_LIBS=-ldl -lrt

CC=gcc
SOFLAGS=-fPIC -shared
CFLAGS=-Wall -O0 -ggdb -std=gnu99 -DDEBUG
RFLAGS=-Wall -O0 -std=gnu99 -DNDEBUG
DFLAGS=-MP -MD

# Library build
BIN=fprof
#FPROF=fprof
LSRC=src
LSRCS=$(wildcard $(LSRC)/fprof*.c)
LOBJ=obj
LOBJS=$(patsubst $(LSRC)/%.c, $(LOBJ)/%.o, $(LSRCS))
DEPFILES=$(patsubst $(LSRC)/%.c, $(LOBJ)/%.d, $(LSRCS))
LIBDIR=bin
LIB=$(LIBDIR)/lib$(BIN).so.$(RVER)

SONAME=lib$(BIN).so.$(SVER)
RNAME=lib$(BIN).so.$(RVER)
LNAME=lib$(BIN).so

HDR_INST=/usr/local/include/$(FPROF)
LIB_INST=/usr/local/lib/$(FPROF)
#CMOD=755

$(shell mkdir -p $(LIBDIR))
$(shell mkdir -p $(LOBJ))

all: lib
release: CFLAGS=$(RFLAGS)
release: lib

lib: $(LIB)

$(LIB): $(LOBJS)
	@echo "=== Building $@ library"
	$(CC) -o $@ $(SOFLAGS) -Wl,-soname,$(SONAME) $(LOBJS) $(OTHER_LIBS)

$(LOBJ)/%.o: $(LSRC)/%.c
	@echo "=== Compiling $<"
	$(CC) $(CFLAGS) $(SOFLAGS) $(DFLAGS) -c $< -o $@ $(OTHER_LIBS)

clean:
	@echo "=== Clean everything"
	$(RM) -r obj/*
	$(RM) -r bin/*

install:
	@echo "=== Installing lib$(BIN)"
	install -d $(HDR_INST)
	install src/fprof.h $(HDR_INST)
	install -d $(LIB_INST)
	install $(LIB) $(LIB_INST)
	ln -srf $(LIB_INST)/$(RNAME) $(LIB_INST)/$(SONAME)
	ln -srf $(LIB_INST)/$(SONAME) $(LIB_INST)/$(LNAME)

-include $(DEPFILES)