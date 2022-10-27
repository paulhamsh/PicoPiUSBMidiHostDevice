cmake_minimum_required(VERSION 3.13)

set(FAMILY rp2040)
include(pico_sdk_import.cmake)
project(pico_host_pio C CXX ASM)
include($ENV{PICO_SDK_PATH}/lib/tinyusb/hw/bsp/family_support.cmake)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()


set(target_name midi_host_dev_ssd)
add_executable(${target_name})

set(PICO_PIO_USB_SRC $ENV{PICO_SDK_PATH}/lib/tinyusb/hw/mcu/raspberry_pi/Pico-PIO-USB/src/)

pico_generate_pio_header(${target_name} ${PICO_PIO_USB_SRC}/usb_tx.pio)
pico_generate_pio_header(${target_name} ${PICO_PIO_USB_SRC}/usb_rx.pio)

set(PICO_RP2040_USB_DEVICE_ENUMERATION_FIX 1)

target_sources(${target_name} PRIVATE
	src/main.c
	src/ssd1306.c
	src/usb_descriptors.c
	${PICO_PIO_USB_SRC}/pio_usb.c
	${PICO_PIO_USB_SRC}/pio_usb_device.c
	${PICO_PIO_USB_SRC}/pio_usb_host.c
	${PICO_PIO_USB_SRC}/usb_crc.c
 	# can use 'tinyusb_pico_pio_usb' library later when pico-sdk is updated
	${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
	${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
)
target_link_options(${target_name} PRIVATE 
	-Xlinker 
	--print-memory-usage
)

target_compile_options(${target_name} PRIVATE 
#	-Wall -Wextra
)

target_include_directories(${target_name} PRIVATE 
	${PICO_PIO_USB_SRC} 
	${CMAKE_CURRENT_LIST_DIR} 
	$ENV{PICO_SDK_PATH}/lib/tinyusb/src/class/audio
	$ENV{PICO_SDK_PATH}/lib/tinyusb/src/class/midi
	src
)

target_link_libraries(${target_name} PRIVATE 
	pico_stdlib  
	pico_multicore 
	hardware_pio 
	hardware_dma 
	hardware_i2c 
	tinyusb_device 
	tinyusb_host 
	tinyusb_board
)
pico_add_extra_outputs(${target_name})


pico_enable_stdio_usb(${target_name} 0)
pico_enable_stdio_uart(${target_name} 1)
