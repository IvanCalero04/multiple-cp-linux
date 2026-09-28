CC      ?= cc
CFLAGS  ?= -O2 -Wall -Wextra -Wpedantic -std=c11
PREFIX  ?= /usr/local

TARGET = mcp

.PHONY: all clean install uninstall test

all: $(TARGET)

$(TARGET): mcp.c
	$(CC) $(CFLAGS) -o $@ $<

install: $(TARGET)
	install -D -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

test: $(TARGET)
	./tests/test.sh

clean:
	rm -f $(TARGET)
