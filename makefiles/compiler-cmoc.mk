CC := cmoc

# CMOC include path for its own headers
CMOC_INCLUDE := /usr/local/share/cmoc/include

# Prevent using macOS system headers - use CMOC's own headers
CFLAGS += -nostdinc -I$(CMOC_INCLUDE)

ASFLAGS += -I src/common -I src/$(CURRENT_PLATFORM) -I src/current-target/$(CURRENT_TARGET)
CFLAGS += -I src/common -I src/$(CURRENT_PLATFORM) -I src/current-target/$(CURRENT_TARGET)

ASFLAGS += -I $(SRCDIR)
CFLAGS += -I $(SRCDIR)

ASFLAGS += -I $(SRCDIR)/include
CFLAGS += -I $(SRCDIR)/include

CFLAGS += -Wno-const

#LIMITFLAGS := --org=0E00 --limit=7C00

LDFLAGS += $(foreach lib, $(LIBS), -L $(dir $(lib)))

LIBFLAGS := $(foreach lib,$(LIBS),-l$(notdir $(lib)))
LIBFLAGS += -lhirestxt

$(OBJDIR)/$(CURRENT_TARGET)/%.o: %.c $(VERSION_FILE) | $(OBJDIR)
	@$(call MKDIR,$(dir $@))
	$(CC) -c $(CFLAGS) -o $@ $<

$(OBJDIR)/$(CURRENT_TARGET)/%.o: %.asm $(VERSION_FILE) | $(OBJDIR)
	@$(call MKDIR,$(dir $@))
	$(CC) -c -o $@ $<

$(BUILD_DIR)/$(PROGRAM_TGT): $(OBJECTS) $(LIBS) | $(BUILD_DIR)
	$(CC) -o $@ $(LIMITFLAGS) $(OBJECTS) $(LDFLAGS) $(LIBFLAGS)
