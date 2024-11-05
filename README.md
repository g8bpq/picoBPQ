Outline spec for microcontroller version of linbpq.

Intended for remote sites where reliability and low power consumption are important.

Cut down version of linbpq with no application interface and only support for KISS TNCs. Includes NETROM and a basic flash file system for configuration files.

I'm currently running it on a Raspberry Pi Pico, which uses an RP2040 chip and has 2MB flash and 264kB RAM. Half of the flash is used for a LittleFS file system which is claimed to be a fail-safe filesystem designed for microcontrollers. There are node commands to upload, list, read, delete and rename files.

Supports up to 6 serial ports, and is primarily intended for use with Nino TNCs without the USB chip, though for testing I'm using a serial to USB Host converter with a standArd USB Nino.

Realtime clock (currently a DS1904). Not really needed as the only thing that uses real time is the MHU command?

Can be accessed via a serial over usb port for initial setup and testing.

If the main config is corrupted the system will create a dummy config with one serial port which should allow access over rf but for emergency use I'm testing a Pimoroni Clipper LTE 4G Breakout for dialup access.






Build Process

Install Arduino IDE

Open up the Arduino IDE and go to File->Preferences.

In the dialog that pops up, enter the following URL in the "Additional Boards Manager URLs" field: https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json

In Boards Manager type RP2040 and select Respberry Pi Pico/RP2040/RP2350 by Earle...

Libraries

install microDS18B20

in Tools/USB Stack - Select AdaFruit TinyUSB

in Tools/Flash Size - Select 1MB FS

Build for me worked but the code did not automatically upload. I had to run Sketch/Export Compiled Binary and set the Pico into Bootsel mode (power up with boot button pressed).

The board appeared as /dev/sdc1 but this may vary. Mount it and copy the compiled .uf2 file to it. On my system it was in

./picoBPQ/build/rp2040.rp2040.rpipico2/picoBPQ.ino.uf2
Outline spec for microcontroller version of linbpq.

Intended for remote sites where reliability and low power consumption are important.

Cut down version of linbpq with no application interface and only support for KISS TNCs. Includes NETROM and a basic flash file system for configuration files.

I'm currently running it on a Raspberry Pi Pico, which uses an RP2040 chip and has 2MB flash and 264kB RAM. Half of the flash is used for a LittleFS file system which is claimed to be a fail-safe filesystem designed for microcontrollers. There are node commands to upload, list, read, delete and rename files.

Supports up to 6 serial ports, and is primarily intended for use with Nino TNCs without the USB chip, though for testing I'm using a serial to USB Host converter with a standArd USB Nino.

Realtime clock (currently a DS1904). Not really needed as the only thing that uses real time is the MHU command?

Can be accessed via a serial over usb port for initial setup and testing.

If the main config is corrupted the system will create a dummy config with one serial port which should allow access over rf but for emergency use I'm testing a Pimoroni Clipper LTE 4G Breakout for dialup access.






Build Process

Install Arduino IDE. See instuctions on Arduino web site. Currently available for Windows or Intel Linux. 

Install RP2040 support:

Open up the Arduino IDE and go to File->Preferences.
In the dialog that pops up, enter the following URL in the "Additional Boards Manager URLs" field: 
https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
In Boards Manager type RP2040 and select Respberry Pi Pico/RP2040/RP2350 by Earle...

Libraries
install microDS18B20

in Tools/Board select Raspberry Pi Pico (or whatever board you are using)
in Tools/USB Stack - Select AdaFruit TinyUSB
in Tools/Flash Size - Select 1MB FS

Clone repo and open picoBPQ.ino in the IDE

On Linux the build worked but the code did not automatically upload. I had to run Sketch/Export Compiled Binary and set the Pico into Bootsel mode (power up with boot button pressed).
The board appeared as /dev/sdc1 but this may vary. Mount it and copy the compiled .uf2 file to it. On my system it was in ./picoBPQ/build/rp2040.rp2040.rpipico2/picoBPQ.ino.uf2

John Wiseman G8BPQ
November 2024
