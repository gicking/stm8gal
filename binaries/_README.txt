This folder contains ready-to-use binaries for the follwoing platforms:

stm8gal_linux64: 
  - built under Xubuntu 18.04 (64bit)
  - with -DUSE_SPIDEV   -> support upload via SPI

stm8gal_linux32
  - built under Xubuntu 18.04 (32bit)
  - with -DUSE_SPIDEV   -> support upload via SPI

stm8gal_raspbian
  - built on RasPi3 under Raspbian Stretch
  - with -DUSE_SPIDEV   -> support upload via SPI
  - skip -DUSE_WIRINGPI -> no reset via GPIO  

stm8gal_windows32.exe
  - built on Windows Vista (32 Bit)
  - skip -DUSE_SPIDEV   -> no upload via SPI (not supported)
  - skip -DUSE_WIRINGPI -> no reset via GPIO and WiringPi  

stm8gal_windows64.exe
  - built on Windows 10 (64 Bit)
  - skip -DUSE_SPIDEV   -> no upload via SPI (not supported)
  - skip -DUSE_WIRINGPI -> no reset via GPIO and WiringPi  
