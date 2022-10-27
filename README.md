# Pico Pi USB Midi Host Device
A MIDI host and device using Pico Pi and PIO USB   

Currently a bit dull - will recognise a 2 USB endpoint USB Midi device plugged into the host, and copy the MIDI messages to the MIDI Device.   
Basically a rudimentary passthough.   

Only tested with a Novation Launchkey 25, a Keith McMillen K Board and a Morninstar MC6 MkI.   
Just as a test this code also sends a colour to the second keypad on the Launchkey 25.   

Uses code adopted from: https://github.com/hathach/tinyusb   
And the SSD 1306 library from: https://github.com/daschr/pico-ssd1306   
And relies on the code from there, which is in the TinyUSB library: https://github.com/sekigon-gonnoc/Pico-PIO-USB   

Creates a MIDI device via the main USB port, and a MIDI host using Pico-PIO-USB.   
The PIO USB is attached to GPIO 6 (pin 9) and GPIO 7 (pin 10)   
The SSD 1306 is attached to GPIO 2 (pin 4) and GPIO 3 (pin 5)  
There is also a serial output on GPIO 0 (pin 1)   

It uses the current TinyUSB library and the Bare API to create the USB Host (NOT reliant on the currently un-merged USB Host MIDI class: https://github.com/rppicomidi/midi2usbhost.git)

As I am learning Pico Pi this is basically just a POC of code I've found. If you want to use it and have questions please raise an issue - I promise to try to help.  


Complete build from scratch intructions (tested on WSL 2 under Windows):   

```
cd ~
mkdir pico_base
cd pico_base

sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone https://github.com/raspberrypi/pico-sdk
cd pico-sdk
git submodule update --init

# For PICO support
cd pico-sdk/lib
rm -R tinyusb
git clone https://github.com/hathach/tinyusb  -b master
cd tinyusb
git submodule update --init --recursive

# This doesn't work
# cd pico_base/pico-sdk/lib/tinyusb/hw/mcu/raspberry_pi
# git submodule update --init


export PICO_SDK_PATH=~/pico_base/pico-sdk  # your SDK location

# And for this program - start here but amend for your PICO_SDK_PATH

############## IMPORTANT MANUAL STEP ##################
# If your PIO-USB is NOT GPIO 2 then manually edit: $PICO_SDK_PATH/lib/tinyusb/hw/bsp/rp2040/family.c
# and change #define PICO_PIO_USB_PIN_DP 2 to 6 (or your own GPIO)

cd ~
git clone https://github.com/paulhamsh/PicoPiUSBMidiHostDevice
cd PicoPiUSBMidiHostDevice/midi_host_dev_ssd
cp ~/pico_base/pico-sdk/external/pico_sdk_import.cmake .
mkdir build
cd build

cmake ..
make



```




