# Pico Pi USB Midi Host Device
A MIDI host and device using Pico Pi and PIO USB   

Currently a bit dull - will recognise a 2 USB endpoint USB Midi device plugged into the host, and copy the MIDI messages to the MIDI Device, so they can be seen by anything you plug the device into.    
Basically a rudimentary passthough.   

Only tested with a Novation Launchkey 25, a Keith McMillen K Board and a Morninstar MC6 MkI.   
Just as a test this code also sends a colour to the second keypad on the Launchkey 25.   

Uses code adopted from: https://github.com/hathach/tinyusb   
And the SSD 1306 library from: https://github.com/daschr/pico-ssd1306   
And relies on the code from here, which is in the TinyUSB library: https://github.com/sekigon-gonnoc/Pico-PIO-USB   

Creates a MIDI device via the main USB port, and a MIDI host using Pico-PIO-USB.   
The PIO USB is attached to GPIO 6 (pin 9) and GPIO 7 (pin 10)   
The SSD 1306 is attached to GPIO 2 (pin 4) and GPIO 3 (pin 5)  
There is also a serial output on GPIO 0 (pin 1)   

It uses the current TinyUSB library and the Bare API to create the USB Host (NOT reliant on the currently un-merged USB Host MIDI class: https://github.com/rppicomidi/midi2usbhost.git)

As I am learning Pico Pi this is basically just a POC of code I've found. If you want to use it and have questions please raise an issue - I promise to try to help.  


Complete build from scratch instructions (scratch meaning - no Pico Pi SDK, nothing - an empty environment) 
Tested on WSL 2 under Windows, which is a good safe test environment because you can just delete the WSL image.   



```
cd ~
mkdir pico_base
cd pico_base

sudo apt install cmake gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
git clone https://github.com/raspberrypi/pico-sdk
cd pico-sdk
git submodule update --init

# For PICO support get latest TinyUSB not the branch used in Pico-SDK
cd lib
rm -R tinyusb
git clone https://github.com/hathach/tinyusb  -b master

cd tinyusb
git submodule update --init --recursive

export PICO_SDK_PATH=~/pico_base/pico-sdk  # your SDK location

# And for this program - start here but amend for your PICO_SDK_PATH

############## IMPORTANT MANUAL STEP ##################
# If your PIO-USB is NOT GPIO 2 then manually edit: $PICO_SDK_PATH/lib/tinyusb/hw/bsp/rp2040/family.c
# and change #define PICO_PIO_USB_PIN_DP 2 to 6 (or your own GPIO)

cd ~
git clone https://github.com/paulhamsh/PicoPiUSBMidiHostDevice
cd PicoPiUSBMidiHostDevice/midi_host_dev_ssd
cp $PICO_SDK_PATH/external/pico_sdk_import.cmake .
mkdir build
cd build

cmake ..
make

```

Then to load to the Pico I use:

```
cp *.uf2 /mnt/c/Users/USER/Desktop/
```

And replace USER with your user name.   
Then copy over to the Pico Pi from Windows proper.  

# Explanation of configuration

## CMakeLists.txt

```
# Standard Pico cmake entries

cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(pico_host_pio C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
pico_sdk_init()

set(target_name midi_host_dev_ssd)
add_executable(${target_name})

# TinyUSB entries
set(FAMILY rp2040)
include($ENV{PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

# Pico PIO USB entries
set(PICO_PIO_USB_SRC $ENV{PICO_SDK_PATH}/lib/tinyusb/hw/mcu/raspberry_pi/Pico-PIO-USB/src/)
pico_generate_pio_header(${target_name} ${PICO_PIO_USB_SRC}/usb_tx.pio)
pico_generate_pio_header(${target_name} ${PICO_PIO_USB_SRC}/usb_rx.pio)

set(PICO_RP2040_USB_DEVICE_ENUMERATION_FIX 1)

target_sources(${target_name} PRIVATE
        # This program
        src/main.c
        src/ssd1306.c
        src/usb_descriptors.c

        # Pico PIO USB
        ${PICO_PIO_USB_SRC}/pio_usb.c
        ${PICO_PIO_USB_SRC}/pio_usb_device.c
        ${PICO_PIO_USB_SRC}/pio_usb_host.c
        ${PICO_PIO_USB_SRC}/usb_crc.c

        # TinyUSB
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
        ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)

target_link_options(${target_name} PRIVATE
        -Xlinker
        --print-memory-usage
)

target_compile_options(${target_name} PRIVATE
#       -Wall -Wextra
)

target_include_directories(${target_name} PRIVATE

        # Standard
        ${CMAKE_CURRENT_LIST_DIR}
        src

        # Pico PIO USB
        ${PICO_PIO_USB_SRC}

        # TinyUSB
        $ENV{PICO_SDK_PATH}/lib/tinyusb/src/class/audio
        $ENV{PICO_SDK_PATH}/lib/tinyusb/src/class/midi
)

target_link_libraries(${target_name} PRIVATE
        # Standard Pico
        pico_stdlib
        pico_multicore
        hardware_pio
        hardware_dma
        hardware_i2c

        # TinyUSB
        tinyusb_device
        tinyusb_host
        tinyusb_board
)

pico_add_extra_outputs(${target_name})

# Turn off stdio USB, and turn on UART
pico_enable_stdio_usb(${target_name} 0)
pico_enable_stdio_uart(${target_name} 1)	
	
```

## Alternative CMakeLists.txt using the TinyUSB 'family'

```
cmake_minimum_required(VERSION 3.5)

include($ENV{PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)

# gets PROJECT name for the example
family_get_project_name(PROJECT ${CMAKE_CURRENT_LIST_DIR})
project(${PROJECT})

# Checks this example is valid for the family and initializes the project
family_initialize_project(${PROJECT} ${CMAKE_CURRENT_LIST_DIR})

add_executable(${PROJECT})

# Example source
target_sources(${PROJECT} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ssd1306.c
        ${CMAKE_CURRENT_SOURCE_DIR}/src/usb_descriptors.c
        )

# Example include
target_include_directories(${PROJECT} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        )

# Configure compilation flags and libraries for the example... see the corresponding function
# in hw/bsp/FAMILY/family.cmake for details.
family_configure_host_example(${PROJECT})
family_configure_device_example(${PROJECT})

# For rp2040, un-comment to enable pico-pio-usb
family_add_pico_pio_usb(${PROJECT})

# due to warnings from other net source, we need to prevent error from some of the warnings options
target_compile_options(${PROJECT} PUBLIC
        -Wno-error=shadow
        -Wno-error=cast-align
        -Wno-error=cast-qual
        -Wno-error=redundant-decls
        -Wno-error=sign-conversion
        -Wno-error=conversion
        -Wno-error=sign-compare
        -Wno-error=unused-function
        -Wno-error=discarded-qualifiers
        -Wno-error=unused-parameter
        -Wno-error=maybe-uninitialized
        -Wno-cast-qual
        -Wno-cast-align
        -Wno-sign-compare
        -Wno-shadow
        -Wno-redundant-decls
        -Wno-unused-function
        )


target_link_libraries(${PROJECT} PRIVATEA
	# for SSD
        hardware_i2c
)

pico_enable_stdio_usb(${PROJECT} 0)
pico_enable_stdio_uart(${PROJECT} 1)
```

## tusb_config.h

tusb_config.h is used to configure the USB library.   The comments starting ```////``` are to show the key parts needed to trigger the required options.   
We are using DEVICE and HOST modes, so need to tell it which RHPORT to use for each.
Then we need to say that TUH and TUD modes are enabled.   
We need to define CFG_TUH_RPI_PIO_USB to enable the PIO-USB capability.   
We define CFG_TUD_MIDI to include the standard class for the MIDI USB device.    


```
#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------+
// Board Specific Configuration
//--------------------------------------------------------------------+

// RHPort number used for host can be defined by board.mk, default to port 0
//#ifndef BOARD_TUH_RHPORT
#define BOARD_TUH_RHPORT      1		//// Set PORT for HUB - Pico PIO USB 
//#endif

// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUH_MAX_SPEED
#define BOARD_TUH_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif


// RHPort number used for device can be defined by board.mk, default to port 0
#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0		//// Set POR for DEVICE
#endif

// RHPort max operational speed can defined by board.mk
#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// defined by compiler flags for flexibility
#ifndef CFG_TUSB_MCU
#error CFG_TUSB_MCU must be defined
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_NONE
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

// Enable Device stack, Default is max speed that hardware controller could support with on-chip PHY
#define CFG_TUD_ENABLED       1			//// Enable DEVICE
#define CFG_TUD_MAX_SPEED     BOARD_TUD_MAX_SPEED


// Enable Host stack
#define CFG_TUH_ENABLED       1			//// Enable HOST
#define CFG_TUH_MAX_SPEED     BOARD_TUH_MAX_SPEED

#if CFG_TUSB_MCU == OPT_MCU_RP2040
// Use pico-pio-usb as host controller for raspberry rp2040
#define CFG_TUH_RPI_PIO_USB   1			//// Enable Pico PIO USB
#endif


// Default is max speed that hardware controller could support with on-chip PHY
#define CFG_TUH_MAX_SPEED     BOARD_TUH_MAX_SPEED

/* USB DMA on some MCUs can only access a specific SRAM region with restriction on alignment.
 * Tinyusb use follows macros to declare transferring memory so that they can be put
 * into those specific section.
 * e.g
 * - CFG_TUSB_MEM SECTION : __attribute__ (( section(".usb_ram") ))
 * - CFG_TUSB_MEM_ALIGN   : __attribute__ ((aligned(4)))
 */
#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN          __attribute__ ((aligned(4)))
#endif


//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif


//------------- CLASS -------------//
#define CFG_TUD_CDC               0
#define CFG_TUD_MSC               0
#define CFG_TUD_HID               0
#define CFG_TUD_MIDI              1		//// To get MIDI device class 
#define CFG_TUD_VENDOR            0

// MIDI FIFO size of TX and RX
#define CFG_TUD_MIDI_RX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)
#define CFG_TUD_MIDI_TX_BUFSIZE   (TUD_OPT_HIGH_SPEED ? 512 : 64)

//--------------------------------------------------------------------
// HOST CONFIGURATION
//--------------------------------------------------------------------

// Size of buffer to hold descriptors and other data used for enumeration
#define CFG_TUH_ENUMERATION_BUFSIZE 256

#define CFG_TUH_HUB                 1		//// Allow for a USB Hub 

// max device support (excluding hub device)
// 1 hub typically has 4 ports
#define CFG_TUH_DEVICE_MAX          (CFG_TUH_HUB ? 4 : 1)

// Max endpoint per device
#define CFG_TUH_ENDPOINT_MAX        8

// Enable tuh_edpt_xfer() API
#define CFG_TUH_API_EDPT_XFER       1

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */

```
