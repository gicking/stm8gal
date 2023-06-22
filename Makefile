# Project: stm8gal

ifeq ($(origin CC), default)
CC = $(CROSS_COMPILE)gcc
endif
CFLAGS        += -c -Wall -I./RAM_Routines/write_erase -I./RAM_Routines/verify_CRC32
#CFLAGS       += -DDEBUG
LDFLAGS       += -g3 -lm
SOURCES       = bootloader.c hexfile.c main.c misc.c serial_comm.c spi_Arduino_comm.c verify_CRC32.c
INCLUDES      = misc.h bootloader.h hexfile.h serial_comm.h spi_spidev_comm.h spi_Arduino_comm.h verify_CRC32.h main.h
RAMINCLUDES   = \
		RAM_Routines/write_erase/erase_write_verL_8k_1.0_inc.h \
		RAM_Routines/write_erase/erase_write_ver_32k_1.0_inc.h \
		RAM_Routines/write_erase/erase_write_ver_32k_1.2_inc.h \
		RAM_Routines/write_erase/erase_write_ver_32k_1.3_inc.h \
		RAM_Routines/write_erase/erase_write_ver_128k_2.0_inc.h \
		RAM_Routines/write_erase/erase_write_ver_128k_2.1_inc.h \
		RAM_Routines/write_erase/erase_write_ver_128k_2.2_inc.h
OBJDIR        = Objects
OBJECTS       = $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCES))
BIN           = stm8gal
ifeq ($(OS),Windows_NT)
	RM = cmd.exe /C del /Q
	MKDIR = mkdir
else
	RM = rm -fr
	MKDIR = mkdir -p
endif

# add optional SPI support via spidev library (Windows not yet supported)
#CFLAGS   += -DUSE_SPIDEV
#SOURCES  += spi_spidev_comm.c

# add optional GPIO reset via wiringPi library (Raspberry only)
#CFLAGS   += -DUSE_WIRING
#LDFLAGS  += -lwiringPi


.PHONY: clean all default objects

.PRECIOUS: $(BIN) $(OBJECTS)

default: $(BIN) $(OBJDIR)

all: $(RAMINCLUDES) $(SOURCES) $(BIN)


# make output directories
$(OBJDIR):
	$(MKDIR) $(OBJDIR)

# clean up
clean:
	${RM} $(OBJECTS) $(OBJDIR) $(BIN) $(BIN).exe *~ .DS_Store

# link application
$(BIN): $(OBJECTS) $(OBJDIR)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# compile all *c files
$(OBJDIR)/%.o: %.c $(SOURCES) $(INCLUDES) $(RAMINCLUDES) $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
