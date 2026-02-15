# PocolC - High-Level Language Compiler Makefile

TARGET = poclc

CC ?= gcc
CFLAGS = --std=c99 -Wall -Wextra -pedantic
PROFLAGS = -O2 -s
DEBUGFLAGS = -g

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    PLATFORM_FLAGS = -D_LINUX_
else ifeq ($(UNAME_S),Darwin)
    PLATFORM_FLAGS = -D_DARWIN_
else
    PLATFORM_FLAGS = -D_WIN32
endif

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

SRCS = poclc.c poclc_main.c
OBJS = $(SRCS:.c=.o)

all: CFLAGS += $(PROFLAGS) $(PLATFORM_FLAGS)
all: $(TARGET)

debug: CFLAGS += $(DEBUGFLAGS) $(PLATFORM_FLAGS)
debug: $(TARGET)

# Linking: .o -> single target
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compiler: .c -> .o object
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

install: $(TARGET)
	install -d $(BINDIR)
	install -m 0755 $(TARGET) $(BINDIR)

uninstall:
	rm -f $(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all debug install uninstall clean
