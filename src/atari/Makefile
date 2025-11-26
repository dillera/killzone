# KillZone Atari 8-bit Client Build System
# Builds C code with cc65 toolchain for Atari 800/XL target

# FujiNet Library
FN_VERSION = 4.8.1
FN_DIR = ../../_cache/fujinet-lib/$(FN_VERSION)-atari
FN_LIB = $(FN_DIR)/fujinet-atari-$(FN_VERSION).lib

# Compiler and tools
CC = cl65
CFLAGS = -t atari -Os --register-vars -I $(FN_DIR)
LDFLAGS = 

# Source files
SOURCES = killzone.c network.c state.c display.c json.c parser.c
HEADERS = network.h state.h display.h json.h parser.h wall_config.h
OBJECTS = $(SOURCES:.c=.o)

# Output
TARGET = client.bin

# Directories
BUILD_DIR = build
SRC_DIR = .

# Phony targets
.PHONY: all clean watch help

# Default target
all: $(TARGET)

# Build target
$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(FN_LIB)
	@echo "Build complete: $(TARGET)"

# Clean build artifacts
clean:
	rm -f $(TARGET) *.o
	@echo "Clean complete"

# Watch mode - rebuild on file changes
watch:
	@echo "Watching for changes (requires fswatch)..."
	@fswatch -r . --exclude build | xargs -I {} make clean all

# Help
help:
	@echo "KillZone Atari Client Build"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Build client binary"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make watch        - Auto-rebuild on file changes"
	@echo "  make help         - Show this help"
	@echo ""
	@echo "After building, run emulator:"
	@echo "  ~/atari800 $(TARGET)"
