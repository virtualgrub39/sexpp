LIBDIR 	?= lib
BUILD 	?= build
CC		?= cc
AR		?= ar

CFLAGS += -Wall -Wextra -pedantic-errors
CFLAGS += -ansi
CFLAGS += -Iinclude
CFLAGS += -fPIC

# CFLAGS += -ggdb
CFLAGS += -O2

STATIC_LIB = $(LIBDIR)/libsexpp.a
SHARED_LIB = $(LIBDIR)/libsexpp.so

.PHONY: all clean

all: $(SHARED_LIB) $(STATIC_LIB) $(BUILD)/test

$(LIBDIR) $(BUILD):
	mkdir -p $@

$(BUILD)/sexpp.o: source/sexpp.c | $(BUILD) include/sexpp.h
	$(CC) -c -o $@ $(CFLAGS) $^

$(SHARED_LIB): $(BUILD)/sexpp.o | $(LIBDIR)
	$(CC) -shared -o $@ $(CFLAGS) $^ $(LDFLAGS)

$(STATIC_LIB): $(BUILD)/sexpp.o | $(LIBDIR)
	$(AR) -rcs $@ $^

$(BUILD)/test: source/test.c $(STATIC_LIB) | $(BUILD)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD) $(LIBDIR)
