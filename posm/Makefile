# PocolVM Assembler (posm) - Makefile
# ======================================

TARGET      = posm
PROJECT     = posm
VERSION     = 1.0.0

CC          ?= gcc
CFLAGS      = --std=c99 -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration
PROFLAGS    = -O2 -DNDEBUG
DEBUGFLAGS  = -g -O0 -DDEBUG

UNAME_S     := $(shell uname -s 2>/dev/null || echo "Unknown")
ifeq ($(UNAME_S),Linux)
    PLATFORM_FLAGS = -D_LINUX_
else ifeq ($(UNAME_S),Darwin)
    PLATFORM_FLAGS = -D_DARWIN_
else
    PLATFORM_FLAGS = -D_WIN32_
endif

PREFIX      ?= /usr/local
BINDIR      = $(PREFIX)/bin
SRCDIR      = .
BUILDDIR    = .

SRCS        = $(filter-out $(SRCDIR)/posm.c, $(wildcard $(SRCDIR)/*.c))
MAIN        = posm.c
OBJS        = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))
DEPS        = $(OBJS:.o=.d)

# Colors
RED     = $(shell tput setaf 1 2>/dev/null || echo "")
GREEN   = $(shell tput setaf 2 2>/dev/null || echo "")
YELLOW  = $(shell tput setaf 3 2>/dev/null || echo "")
BLUE    = $(shell tput setaf 4 2>/dev/null || echo "")
RESET   = $(shell tput sgr0 2>/dev/null || echo "")

.PHONY: all
all: info $(BUILDDIR)/$(TARGET)

.PHONY: info
info:
	@echo "$(BLUE)=== $(PROJECT) v$(VERSION) ===$(RESET)"

$(BUILDDIR)/$(TARGET): $(OBJS) $(MAIN)
	@echo "$(GREEN)Building $(TARGET)...$(RESET)"
	$(CC) $(CFLAGS) $(PROFLAGS) $(PLATFORM_FLAGS) $(MAIN) $(OBJS) -o $@
	@echo "$(GREEN)Built successfully!$(RESET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) $(PROFLAGS) $(PLATFORM_FLAGS) -MMD -MP -c $< -o $@

.PHONY: debug
debug: CFLAGS += $(DEBUGFLAGS)
debug: clean
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(PLATFORM_FLAGS) $(MAIN) $(OBJS) -o $(BUILDDIR)/$(TARGET)_debug

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/*.d $(BUILDDIR)/$(TARGET) $(BUILDDIR)/$(TARGET)_debug

.PHONY: install
install: $(BUILDDIR)/$(TARGET)
	install -d $(BINDIR)
	install -m 0755 $(BUILDDIR)/$(TARGET) $(BINDIR)/

.PHONY: uninstall
uninstall:
	rm -f $(BINDIR)/$(TARGET)

.PHONY: help
help:
	@echo "$(BLUE)=== $(PROJECT) Makefile ===$(RESET)"
	@echo "  make       - Build release"
	@echo "  make debug - Build debug"
	@echo "  make clean - Clean"
	@echo "  make help  - Help"

-include $(DEPS)
