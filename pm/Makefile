TARGET = pm

CC ?= gcc
CFLAGS = --std=c99 -MMD -MP -Wall -Wextra
EXTRAFLAGS ?= # extra flags from input
PROFLAGS = -O2 -s
DEBUGFLAGS = -g

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)

-include $(SRCS:.c=.d)

all: CFLAGS += $(PROFLAGS)
all: $(TARGET)

debug: CFLAGS += $(DEBUGFLAGS)
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
	rm -f $(TARGET) $(OBJS) $(SRCS:.c=.d)

.PHONY: all debug install uninstall clean
