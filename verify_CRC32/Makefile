# toolchain settings
CC = sdcc
CFLAGS = -mstm8 --opt-code-size --max-allocs-per-node 25000
AS = sdasstm8
ASFLAGS = -o
LD = sdldstm8
LDFLAGS = -n

# common directories
LINKDIR = link
OBJDIR = obj
BINDIR = bin
OUTPUT_HEADERDIR = include

# linker files. Each linker file generates one output binary
LINKTARGETS = \
	$(LINKDIR)/STM8L_8k_v1.0.lk \
	$(LINKDIR)/STM8L_32k_v1.1.lk \
	$(LINKDIR)/STM8L_32k_v1.2.lk \
	$(LINKDIR)/STM8L_64k_v1.1.lk \
	$(LINKDIR)/STM8S_32k_v1.2.lk \
	$(LINKDIR)/STM8S_32k_v1.3.lk \
	$(LINKDIR)/STM8S_128k_v2.1.lk \
	$(LINKDIR)/STM8S_128k_v2.2.lk
LINKCOMMON = \
	$(LINKDIR)/common.lk

# output hexfiles and converted headers
BINARIES = $(patsubst $(LINKDIR)/%.lk,$(BINDIR)/verify_CRC32_%.ihx,$(LINKTARGETS))

# project files
PRJ_SRC_DIR = .
PRJ_INC_DIR = $(PRJ_SRC_DIR)
PRJ_SOURCES = $(foreach d, $(PRJ_SRC_DIR), $(wildcard $(d)/*.c))
PRJ_HEADERS = $(foreach d, $(PRJ_INC_DIR), $(wildcard $(d)/*.h))
PRJ_OBJECTS = $(addprefix $(OBJDIR)/, $(notdir $(PRJ_SOURCES:.c=.rel)))

# CRC32 library files
CRC_SRC_DIR = ./stm8-crc
CRC_INC_DIR = $(CRC_SRC_DIR)
CRC_SOURCES = $(foreach d, $(CRC_SRC_DIR), $(wildcard $(d)/*.c))
CRC_HEADERS = $(foreach d, $(CRC_INC_DIR), $(wildcard $(d)/*.h))
CRC_OBJECTS = $(addprefix $(OBJDIR)/, $(notdir $(CRC_SOURCES:.c=.rel)))

# concat all project files
SRC_DIR     = $(PRJ_SRC_DIR) $(CRC_SRC_DIR)
INC_DIR     = $(PRJ_INC_DIR) $(CRC_INC_DIR)
SOURCES     = $(PRJ_SOURCES) $(CRC_SOURCES)
HEADERS     = $(PRJ_HEADERS) $(CRC_HEADERS)
OBJECTS     = $(PRJ_OBJECTS) $(CRC_OBJECTS)

# set compiler include paths
INCLUDE          = $(foreach d, $(INC_DIR), $(addprefix -I, $(d)))

# set make search paths
vpath %.c $(SRC_DIR)
vpath %.h $(INC_DIR)

# debug: print variable and stop
#$(error variable is [${SOURCES}])

# OS dependent commands
ifeq ($(OS),Windows_NT)
	RM = cmd.exe /C del /Q
	MKDIRFLAGS =
else
	RM = rm -fr
	MKDIRFLAGS = -p
endif

# set make search paths
vpath %.c ./stm8-crc


.PHONY: all clean

all: $(BINARIES)

$(BINARIES): $(LINKCOMMON) $(LINKTARGETS) $(OBJECTS) $(BINDIR) $(OUTPUT_HEADERDIR)
	$(LD) $(LDFLAGS) -i $@ -f $(LINKCOMMON) -f $(LINKDIR)/$(patsubst verify_CRC32_%.ihx,%.lk,$(notdir $@))
ifeq ($(OS),Windows_NT)
	check-seg-size.bat $(BINDIR)/$(basename $(@F)).map
else
	awk -f ./check-seg-size.awk $(BINDIR)/$(basename $(@F)).map
	xxd -i $@ > $(OUTPUT_HEADERDIR)/$(basename $(@F)).h
	$(RM) ../RAM_Routines/verify_CRC32/$(basename $(@F)).h
	cp $(OUTPUT_HEADERDIR)/$(basename $(@F)).h ../RAM_Routines/verify_CRC32/$(basename $(@F)).h
	echo
endif

$(OBJECTS): $(SOURCES) $(OBJDIR)

$(OBJDIR)/%.rel: %.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(OBJDIR)/%.rel: %.s
	$(AS) $(ASFLAGS) $@ $(basename $(<F)).s

$(OBJDIR):
	mkdir $(MKDIRFLAGS) $(OBJDIR)

$(BINDIR):
	mkdir $(MKDIRFLAGS) $(BINDIR)

$(OUTPUT_HEADERDIR):
	mkdir $(MKDIRFLAGS) $(OUTPUT_HEADERDIR)

clean:
	$(RM) $(OBJDIR)
	$(RM) $(BINDIR)
	$(RM) $(OUTPUT_HEADERDIR)
