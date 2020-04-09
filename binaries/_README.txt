This folder contains ready-to-use binaries for the following platforms:

stm8gal_linux64: 
  - built under Xubuntu 18.04 (64 bit)
  - without -DUSE_SPIDEV   -> support upload via SPI. See Makefile
  - without -DUSE_WIRINGPI -> no reset via GPIO and WiringPi (not supported)

stm8gal_linux32
  - built under Xubuntu 18.04 (32 bit)
  - without -DUSE_SPIDEV   -> support upload via SPI. See Makefile
  - without -DUSE_WIRINGPI -> no reset via GPIO and WiringPi (not supported)

stm8gal_raspbian32
  - built on RasPi3 under Raspbian 9 Stretch (32 bit)
  - without -DUSE_SPIDEV   -> support upload via SPI. See Makefile
  - without -DUSE_WIRINGPI -> no reset via GPIO. See Makefile

stm8gal_windows32.exe
  - built on Windows 7 (32 bit)
  - without -DUSE_SPIDEV   -> no upload via SPI (not supported)
  - without -DUSE_WIRINGPI -> no reset via GPIO and WiringPi (not supported)

stm8gal_windows64.exe
  - built on Windows 10 (64 bit)
  - without -DUSE_SPIDEV   -> no upload via SPI (not supported)
  - without -DUSE_WIRINGPI -> no reset via GPIO and WiringPi (not supported)
