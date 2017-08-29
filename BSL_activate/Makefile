#######################
# SDCC Makefile for making a hexfile from all .C files in this directory.
# Output files are generated in a directory with name of device.
#######################

DEVICE = STM8S105
#DEVICE = STM8S207

# comm port for serial flasher. For STM8S Discovery Board use UART
ifeq ($(DEVICE),STM8S105)
	PORT = /dev/ttyAMA0
else
	PORT = /dev/ttyUSB0
endif	

# define compiler (has to be in PATH)
CC = sdcc

# define output path, compiler/linker options etc.
CFLAGS = -mstm8 --std-sdcc99 -D$(DEVICE)
LFLAGS = -mstm8 -lstm8 $(OPTIMIZE) --out-fmt-ihx
OBJDIR = $(DEVICE)
OBJECTS = $(patsubst %.c, $(OBJDIR)/%.rel, $(wildcard *.c))
HEADERS = $(wildcard *.h)

# set defaults 
TARGET=$(OBJDIR)/main.ihx

.PHONY: clean all default objects

.PRECIOUS: $(TARGET) $(OBJECTS)

default: $(TARGET)

all: default

# create output folder
$(OBJDIR):
	mkdir -p $(OBJDIR)

# compile all *c files
$(OBJDIR)/%.rel: %.c $(HEADERS) $(OBJDIR)
	$(CC) $(CFLAGS) -I./$(MULIB) -c $< -o $@

# link all object files and libaries
$(TARGET): $(OBJECTS)
	$(CC) $(LFLAGS) $(OBJECTS) -o $@
	
# clean up
clean:
	rm -fr .DS_Store
	rm -fr $(OBJDIR)
	rm -fr Release
	rm -fr Debug

# upload via stm8flash / SWIM (https://github.com/vdudouyt/stm8flash)
# stm8flash requires device in lower-case -> http://gnu-make.2324884.n4.nabble.com/how-to-achieve-conversion-to-uppercase-td14496.html 
swim:
	stm8flash -c stlink -w $(TARGET) -p $(shell echo $(DEVICE) | tr A-Z a-z)

# upload via UART bootloader (https://github.com/gicking/stm8gal)
serial:
ifeq ($(PORT),/dev/ttyAMA0)
	../stm8gal -p $(PORT) -f $(TARGET) -q -Q -r -u 2
else
	../stm8gal -p $(PORT) -f $(TARGET) -q
endif	

