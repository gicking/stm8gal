CC = sdcc
CFLAGS = -mstm8 --opt-code-size --max-allocs-per-node 25000
AS = sdasstm8
ASFLAGS = -o
LD = sdldstm8
LDFLAGS = -n
SOURCES = \
	verify_CRC32.c \
	verify_CRC32.h
OBJDIR = obj
OBJECTS = $(patsubst %,$(OBJDIR)/%.rel,$(basename $(filter %.c %.s,$(SOURCES))))
LINKDIR = link
# TODO: use $(wildcard ...) to auto-populate this list from link dir contents?
LINKTARGETS = \
	$(LINKDIR)/linker.lk
LINKCOMMON = \
	$(LINKDIR)/common.lk
BINDIR = bin
BINARY = $(BINDIR)/verify_CRC32.ihx
HEADERDIR = include
HEADER = $(HEADERDIR)/verify_CRC32_ihx.h
ifeq ($(OS),Windows_NT)
	RM = cmd.exe /C del /Q
	MKDIRFLAGS =
else
	RM = rm -fr
	MKDIRFLAGS = -p
endif

.PHONY: all clean

all: $(BINARY) $(HEADER)

$(BINARY): $(LINKCOMMON) $(LINKTARGETS) $(OBJECTS) $(BINDIR) $(HEADERDIR)
	$(LD) $(LDFLAGS) -i $@ -f $(LINKCOMMON) -f $(LINKTARGETS)
ifeq ($(OS),Windows_NT)
	check-seg-size.bat $(BINDIR)/$(basename $(@F)).map
else
	awk -f ./check-seg-size.awk $(BINDIR)/$(basename $(@F)).map
endif

$(HEADER): $(BINARY)
	xxd -i $(BINARY) > $(HEADER)
	cp $(HEADER) ../RAM_Routines

$(OBJECTS): $(SOURCES) $(OBJDIR)

$(OBJDIR)/%.rel: %.c
	$(CC) $(CFLAGS) -o $@ -c $(basename $(<F)).c

$(OBJDIR)/%.rel: %.s
	$(AS) $(ASFLAGS) $@ $(basename $(<F)).s

$(OBJDIR):
	mkdir $(MKDIRFLAGS) $(OBJDIR)

$(BINDIR):
	mkdir $(MKDIRFLAGS) $(BINDIR)

$(HEADERDIR):
	mkdir $(MKDIRFLAGS) $(HEADERDIR)

clean:
	$(RM) $(OBJDIR)
	$(RM) $(BINDIR)
	$(RM) $(HEADERDIR)
