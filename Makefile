# Project: stm8gal

CC            = gcc
CFLAGS        = -c -Wall -I./STM8_Routines
#CFLAGS       += -DDEBUG
LDFLAGS       = -g3 -lm
SOURCES       = bootloader.c hexfile.c main.c console.c timer.c serial_comm.c
INCLUDES      = bootloader.h hexfile.h main.h console.h timer.h serial_comm.h
STM8FLASH     = STM8_Routines/E_W_ROUTINEs_128K_ver_2.1.s19 STM8_Routines/E_W_ROUTINEs_128K_ver_2.0.s19 STM8_Routines/E_W_ROUTINEs_256K_ver_1.0.s19 STM8_Routines/E_W_ROUTINEs_32K_ver_1.3.s19 STM8_Routines/E_W_ROUTINEs_32K_ver_1.4.s19 STM8_Routines/E_W_ROUTINEs_128K_ver_2.2.s19 STM8_Routines/E_W_ROUTINEs_32K_ver_1.0.s19 STM8_Routines/E_W_ROUTINEs_128K_ver_2.4.s19 STM8_Routines/E_W_ROUTINEs_32K_ver_1.2.s19  STM8_Routines/E_W_ROUTINEs_32K_verL_1.0.s19 STM8_Routines/E_W_ROUTINEs_8K_verL_1.0.s19
STM8INCLUDES  = $(STM8FLASH:.s19=.h)
OBJDIR        = Objects
OBJECTS       = $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCES))
BIN           = stm8gal
RM            = rm -fr

# add optional SPI support via spidev library (Windows not yet supported)
#CFLAGS   += -DUSE_SPIDEV
#SOURCES  += spi_spidev_comm.c
#INCLUDES += spi_spidev_comm.h

# add optional SPI support via Arduino USB<->SPI bridge (see )
CFLAGS   += -DUSE_SPI_ARDUINO
SOURCES  += spi_Arduino_comm.c
INCLUDES += spi_Arduino_comm.h

# add optional GPIO reset via wiringPi library (Raspberry only) 
#CFLAGS   += -DUSE_WIRING
#LDFLAGS  += -lwiringPi


.PHONY: clean all default objects

.PRECIOUS: $(BIN) $(OBJECTS)

default: $(BIN) $(OBJDIR)

all: $(STM8INCLUDES) $(SOURCES) $(BIN)
	
$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	${RM} $(OBJECTS) $(OBJDIR) $(BIN) $(BIN).exe *~ .DS_Store 
	
%.h: %.s19 $(STM8FLASH)
	xxd -i $< > $@
	  
# link application
$(BIN): $(OBJECTS) $(OBJDIR)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

# compile all *c files
$(OBJDIR)/%.o: %.c $(SOURCES) $(INCLUDES) $(STM8INCLUDES) $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@
