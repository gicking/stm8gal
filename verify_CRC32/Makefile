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
# TODO: use $(wildcard ...) to auto-populate this list from link dir contents?
LINKTARGETS = \
	./linker.lk
BINDIR = bin
BINARY = $(BINDIR)/verify_CRC32.ihx
HEADER = $(BINDIR)/verify_CRC32_ihx.h
ifeq ($(OS),Windows_NT)
	RM = cmd.exe /C del /Q
	MKDIRFLAGS =
else
	RM = rm -fr
	MKDIRFLAGS = -p
endif

.PHONY: all clean

all: $(BINARY) $(HEADER)

$(BINARY): $(LINKTARGETS) $(OBJECTS) $(BINDIR)
	$(LD) $(LDFLAGS) -i $@ -f $(LINKTARGETS)

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

clean:
	$(RM) $(OBJDIR)
	$(RM) $(BINDIR)
