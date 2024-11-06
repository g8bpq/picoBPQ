Outline spec for microcontroller version of linbpq.

Intended for remote sites where reliability and low power consumption are important.

Cut down version of linbpq with no application interface and only support for KISS TNCs. Includes NETROM and a basic flash file system for configuration files.

I'm currently running it on a Raspberry Pi Pico, which uses an RP2040 chip and has 2MB flash and 264kB RAM. Half of the flash is used for a LittleFS file system which is claimed to be a fail-safe filesystem designed for microcontrollers. There are node commands to upload, list, read, delete and rename files.

Supports up to 6 serial ports, and is primarily intended for use with Nino TNCs without the USB chip, though for testing I'm using a serial to USB Host converter with a standArd USB Nino.

Realtime clock (currently a DS1904). Not really needed as the only thing that uses real time is the MHU command?

Can be accessed via a serial over usb port for initial setup and testing.

If the main config is corrupted the system will create a dummy config with one serial port which should allow access over rf but for emergency use I'm testing a Pimoroni Clipper LTE 4G Breakout for dialup access.

Prebuilt binaries for Pi Pico and Pi Pico 2 are available or you can build from source.


Build Process.

Install Arduino IDE

Open up the Arduino IDE and go to File->Preferences.

In the dialog that pops up, enter the following URL in the Additional Boards Manager URLs field: https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

In Boards Manager type RP2040 and select Respberry Pi Pico/RP2040/RP2350 by Earle...

Libraries

install microDS18B20

in Tools/Board select Raspberry Pi Pico (or whatever board you are using)
in Tools/USB Stack - Select AdaFruit TinyUSB
in Tools/Flash Size - Select 1MB FS


On Linux the build worked for me worked but the code did not automatically upload. I had to run Sketch/Export Compiled Binary and set the Pico into Bootsel mode (power up with boot button pressed).

The board appeared as /dev/sdc1 but this may vary. Mount it and copy the compiled .uf2 file to it. On my system it was in

./picoBPQ/build/rp2040.rp2040.rpipico2/picoBPQ.ino.uf2


Once running a USB Serial port will be created on a connected host. This appears as /dev/ttyACMx on Linux or COMxx on Windows. The node can be managed via a serial terminal using this port.  

Managing Files.

The file system commands are FILES (list dir) READ UPLOAD DELETE RENAME. You can upload a new bpq32.cfg via a terminal on the USB serial port. send UPLOAD then paste a config. It didn't work with the terminal in the iDE but it was fine with putty. You can read the default config with READ bpq32.cg. Commands are sysop only, so you need to enter PASSWORD first. The USB term is considered secure so you don't need to enter password info. Just PASSWORD should do it. 

Serial ports.

Supports the hardware Serial1 and Serial2 and 4 emulated ports using the SerialPIO module. See https://www.raspberrypi.com/news/what-is-pio/ for some info about the RP2040 PIO subsystem - it is used here to create additional high performance serial ports.

Serial1 - TX GP0,  Pico Pin 1,  RX GP1.  Pico Pin 2
Serial2 - TX GP8,  Pico Pin 11, RX GP9.  Pico Pin 12
Serial3 - TX GP2,  Pico Pin 4,  RX GP3.  Pico Pin 5
Serial4 - TX GP4,  Pico Pin 6,  RX GP5.  Pico Pin 7
Serial5 - TX GP6,  Pico Pin 9,  RX GP7.  Pico Pin 10
Serial6 - TX GP10, Pico Pin 14, RX GP11. Pico Pin 15

You can also create two additional serial devices on the USB port. These aren't needed on a live system but I find them useful for testing. One is configured as a second console and the other a KISS port.

Basic bpq32.cfg port definition:

PORT
TYPE=ASYNC
ID=KISS Serial1
COMPORT=Serial1
SPEED=57600
QUALITY=192
ENDPORT

Real TIme Clock

Current support is for the DS1904 chip, mainly because I had a couple lying around. This uses the 1 Wire interface and connects to GP15 (Pin 20). It needs a pullup of around 4.7K to the 3.3v line. I'm planning to add support for the more common DS1307 i2c interface device as well.

Default Config

On first boot, or if neither bpq32.cfg nor backup.cfg contain a valid config the following default config file is created. If there is an invalid bpq32.cfg it is fist renamed bpq32.cfg.bad

  SIMPLE
  NODECALL=DUMMY
  NODEALIAS=RP2040
  PASSWORD=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
  
  PORT
  TYPE=INTERNAL
  ENDPORT
  
  PORT
  TYPE=ASYNC
  ID=KISS Serial1
  COMPORT=Serial1
  SPEED=57600
  QUALITY=0
  ENDPORT;

If you are building from source it would be a good idea to change the NODECALL and PASSWORD



