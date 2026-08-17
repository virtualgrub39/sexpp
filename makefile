SOURCE += source/sexpp.c
LIBDIR = lib
BUILD = build

CFLAGS += -Wall -Wextra
CFLAGS += -Iinclude
CFLAGS += -std=c89
CFLAGS += -ggdb
# CFLAGS += -O2

all: $(LIBDIR)/sexpp.so $(LIBDIR)/sexpp.a $(BUILD)/test

$(LIBDIR) $(BUILD):
	mkdir -p $@

$(BUILD)/sexpp.o: $(SOURCE) | $(BUILD)
	$(CC) -c -o $@ $(CFLAGS) $^

$(LIBDIR)/sexpp.so: $(BUILD)/sexpp.o | $(LIBDIR)
	$(CC) -shared -fPIC -o $@ $(CFLAGS) $^ $(LDFLAGS)

$(LIBDIR)/sexpp.a: $(BUILD)/sexpp.o | $(LIBDIR)
	$(AR) -rcs $@ $^

$(BUILD)/test: source/test.c $(LIBDIR)/sexpp.a | $(BUILD)
	$(CC) -o $@ $(CFLAGS) $^ $(LDFLAGS)
