# PocolVM - Makefile
# =================

# Project info
TARGET      = pm
PROJECT     = PocolVM
VERSION     = 1.0.0

# Compiler
CC          ?= gcc
CFLAGS      = --std=c99 -Wall -Wextra -Wpedantic -Werror=implicit-function-declaration
LDFLAGS     =

# Build options
PROFLAGS    = -O2 -DNDEBUG
DEBUGFLAGS  = -g -O0 -DDEBUG
COVERAGE    = --coverage

# Platform detection
UNAME_S     := $(shell uname -s 2>/dev/null || echo "Unknown")
PLATFORM_FLAGS =
ifeq ($(UNAME_S),Linux)
    PLATFORM_FLAGS += -D_LINUX_
else ifeq ($(UNAME_S),Darwin)
    PLATFORM_FLAGS += -D_DARWIN_
else
    PLATFORM_FLAGS += -D_WIN32_
endif

# Directories
PREFIX      ?= /usr/local
BINDIR      = $(PREFIX)/bin
LIBDIR      = $(PREFIX)/lib
INCLUDEDIR  = $(PREFIX)/include
SRCDIR      = .
BUILDDIR    = .
TESTSDIR    = tests
BINDIR      = bin

# Source files
SRCS        = $(filter-out $(SRCDIR)/pm.c, $(wildcard $(SRCDIR)/*.c))
MAIN        = pm.c
OBJS        = $(patsubst $(SRCDIR)/%.c, $(BUILDDIR)/%.o, $(SRCS))
DEPS        = $(OBJS:.o=.d)

# Test files
TEST_SRCS   = $(wildcard $(TESTSDIR)/*.c)
TESTS       = $(patsubst $(TESTSDIR)/%.c, $(BUILDDIR)/test_%, $(TEST_SRCS))

# Colors
RED     = $(shell tput setaf 1 2>/dev/null || echo "")
GREEN   = $(shell tput setaf 2 2>/dev/null || echo "")
YELLOW  = $(shell tput setaf 3 2>/dev/null || echo "")
BLUE    = $(shell tput setaf 4 2>/dev/null || echo "")
RESET   = $(shell tput sgr0 2>/dev/null || echo "")

# Default target
.PHONY: all
all: info $(BUILDDIR)/$(TARGET)

# Build info
.PHONY: info
info:
	@echo "$(BLUE)=== $(PROJECT) v$(VERSION) ===$(RESET)"
	@echo "Platform: $(UNAME_S)"
	@echo "CC: $(CC)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "Build type: Release"

# Build target
$(BUILDDIR)/$(TARGET): $(OBJS) $(MAIN)
	@echo "$(GREEN)Building $(TARGET)...$(RESET)"
	$(CC) $(CFLAGS) $(PROFLAGS) $(PLATFORM_FLAGS) $(MAIN) $(OBJS) -o $@ $(LDFLAGS)
	@echo "$(GREEN)Built successfully!$(RESET)"

# Build object files
$(BUILDDIR)/%.o: $(SRCDIR)/%.c
	@echo "Compiling: $<"
	$(CC) $(CFLAGS) $(PROFLAGS) $(PLATFORM_FLAGS) -MMD -MP -c $< -o $@

# Debug build
.PHONY: debug
debug: CFLAGS += $(DEBUGFLAGS)
debug: clean
	@echo "$(YELLOW)Building debug version...$(RESET)"
	$(CC) $(CFLAGS) $(DEBUGFLAGS) $(PLATFORM_FLAGS) $(MAIN) $(OBJS) -o $(BUILDDIR)/$(TARGET)_debug $(LDFLAGS)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "$(YELLOW)Cleaning...$(RESET)"
	rm -rf $(BUILDDIR)/*.o $(BUILDDIR)/*.d $(BUILDDIR)/$(TARGET) $(BUILDDIR)/$(TARGET)_debug $(BUILDDIR)/test_* $(TESTSDIR)/*.out
	@echo "$(GREEN)Clean complete!$(RESET)"

# Install
.PHONY: install
install: $(BUILDDIR)/$(TARGET)
	@echo "$(GREEN)Installing $(TARGET)...$(RESET)"
	install -d $(BINDIR)
	install -m 0755 $(BUILDDIR)/$(TARGET) $(BINDIR)/
	@echo "$(GREEN)Installed to $(BINDIR)/$(TARGET)$(RESET)"

# Uninstall
.PHONY: uninstall
uninstall:
	@echo "$(YELLOW)Uninstalling...$(RESET)"
	rm -f $(BINDIR)/$(TARGET)
	@echo "$(GREEN)Uninstalled!$(RESET)"

# Run tests
.PHONY: test
test: $(BUILDDIR)/$(TARGET)
	@echo "$(BLUE)=== Running Tests ===$(RESET)"
	@echo "Run: ./pm/pm <program.pob>"
	@echo "Or use: make test-assembler make test-vm"

# Build assembler (posm)
.PHONY: assembler
assembler:
	$(MAKE) -C ../posm

# Test assembler
.PHONY: test-assembler
test-assembler: assembler
	@echo "$(BLUE)Testing assembler...$(RESET)"
	../posm/posm ../example/3010.pcl -o /tmp/test.pob && echo "$(GREEN)Assembler test passed!$(RESET)" || echo "$(RED)Assembler test failed!$(RESET)"

# Test VM
.PHONY: test-vm
test-vm: $(BUILDDIR)/$(TARGET)
	@echo "$(BLUE)Testing VM...$(RESET)"
	@echo "Run manually: ./pm/pm <program.pob>"

# Format code
.PHONY: format
format:
	@echo "$(BLUE)Formatting code...$(RESET)"
	@command -v clang-format >/dev/null 2>&1 && clang-format -i $(SRCDIR)/*.c $(SRCDIR)/*.h || echo "clang-format not found"

# Lint code
.PHONY: lint
lint:
	@echo "$(BLUE)Linting code...$(RESET)"
	@command -v clang-tidy >/dev/null 2>&1 && clang-tidy $(SRCDIR)/*.c -- -std=c99 -Wall || echo "clang-tidy not found"

# Show help
.PHONY: help
help:
	@echo "$(BLUE)=== $(PROJECT) Makefile Help ===$(RESET)"
	@echo ""
	@echo "$(GREEN)Build:$(RESET)"
	@echo "  make              - Build release version"
	@echo "  make debug        - Build debug version"
	@echo "  make clean        - Clean build artifacts"
	@echo ""
	@echo "$(GREEN)Testing:$(RESET)"
	@echo "  make test         - Show test info"
	@echo "  make test-assembler - Test assembler"
	@echo "  make test-vm     - Test VM"
	@echo ""
	@echo "$(GREEN)Install:$(RESET)"
	@echo "  make install     - Install binary"
	@echo "  make uninstall   - Uninstall binary"
	@echo ""
	@echo "$(GREEN)Code Quality:$(RESET)"
	@echo "  make format      - Format code (clang-format)"
	@echo "  make lint        - Lint code (clang-tidy)"
	@echo ""
	@echo "$(GREEN)Other:$(RESET)"
	@echo "  make info        - Show build info"
	@echo "  make help        - Show this help"

# Include dependencies
-include $(DEPS)
