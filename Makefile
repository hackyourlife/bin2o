#!/bin/make
export	PREFIX	:=	
export	CC	:=	$(PREFIX)gcc
export	LD	:=	$(CC)
export	OBJCOPY	:=	$(PREFIX)objcopy
export	NM	:=	$(PREFIX)nm
export	SIZE	:=	$(PREFIX)size

#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------
TARGET		:=	bin2o
INCLUDES	:=	include
SOURCES		:=	src
BUILD		:=	build

CFLAGS		:=	-O3 -g -Wall -Wextra -std=c99 \
			-D_POSIX_C_SOURCE=200809L \
			-ffunction-sections -fdata-sections \
			$(INCLUDE) -DUNIX #-fsanitize=address

LDFLAGS		:=	-Wl,-x -Wl,--gc-sections #-fsanitize=address

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))

ifneq ($(BUILD),$(notdir $(CURDIR)))
#-------------------------------------------------------------------------------
export	DEPSDIR	:=	$(CURDIR)/$(BUILD)
export	OFILES	:=	$(CFILES:.c=.o)
export	VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(CURDIR)
export	INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
				-I$(CURDIR)/$(BUILD)
export	OUTPUT	:=	$(CURDIR)/$(TARGET)

.PHONY: $(BUILD) clean all

$(BUILD):
	@echo compiling...
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo "[CLEAN]"
	@rm -rf $(BUILD) $(TFILES) $(OFILES) $(TARGET)

$(TARGET): $(TFILES)

else

#-------------------------------------------------------------------------------
# main target
#-------------------------------------------------------------------------------
.PHONY: all

all: $(OUTPUT)

$(OUTPUT): $(TARGET).elf
	@cp $(TARGET).elf $(OUTPUT)

%.o: %.c
	@echo "[CC]    $(notdir $@)"
	@$(CC) -MMD -MP -MF $(DEPSDIR)/$*.d $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OFILES)
	@echo "[LD]    $(notdir $@)"
	@$(LD) $(LDFLAGS) $(OFILES) -o $@ -Wl,-Map=$(@:.elf=.map)

-include $(DEPSDIR)/*.d

#-------------------------------------------------------------------------------
endif
#-------------------------------------------------------------------------------
