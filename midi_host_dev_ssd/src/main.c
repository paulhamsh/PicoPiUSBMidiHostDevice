/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

// Edit pico-sdk/lib/tinyusb/hw/bsp/rp2040/family.c and change #define PICO_PIO_USB_PIN_DP 2 to 6 (or your own GPIO)

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bsp/board_api.h"
#include "tusb.h"
#include "pio_usb.h"


#include "ssd1306.h"
ssd1306_t disp;

tusb_desc_device_t desc_device;
uint8_t dev_addr;
uint8_t in_endpt;
uint8_t out_endpt;

uint8_t in_buf[64];
uint8_t out_buf[64];

uint8_t buf_setup[8] = {0x19, 0x9f, 0x0c, 0x7f, 0x19, 0x9f, 0x61, 0x21};

uint8_t done_startup = 0;

uint8_t note_buf[3];
int got_midi;

//--------------------------------------------------------------------+
// Setup SSD 
//--------------------------------------------------------------------+

void setup_display(void) {
   i2c_init(i2c1, 400000);
   gpio_set_function(2, GPIO_FUNC_I2C);
   gpio_set_function(3, GPIO_FUNC_I2C);
   gpio_pull_up(2);
   gpio_pull_up(3);

   disp.external_vcc=false;
   ssd1306_init(&disp, 128, 64, 0x3C, i2c1);
   ssd1306_clear(&disp);
   ssd1306_draw_string(&disp, 0, 0, 2, "MIDIHOST");
   ssd1306_show(&disp);
}

//--------------------------------------------------------------------+
// Blinking Task
//--------------------------------------------------------------------+

void led_blinking_task(void)
{
  const uint32_t interval_ms = 1000;
  static uint32_t start_ms = 0;

  static bool led_state = false;

  // Blink every interval ms
  if ( board_millis() - start_ms < interval_ms) return; // not enough time
  start_ms += interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}

/*********************************************************************/
// MIDI HOST
/*********************************************************************/

//--------------------------------------------------------------------+
// MIDI Host Interface
//--------------------------------------------------------------------+

void data_sent(tuh_xfer_t* xfer)
{
  if (xfer->result == XFER_RESULT_SUCCESS)
  {
    printf("Data sent success\n");
  }
}

void data_received(tuh_xfer_t* xfer)
{
  // Note: not all field in xfer is available for use (i.e filled by tinyusb stack) in callback to save sram
  // For instance, xfer->buffer is NULL. We have used user_data to store buffer when submitted callback
  uint8_t* buf = (uint8_t*) xfer->user_data;

  if (xfer->result == XFER_RESULT_SUCCESS)
  {
    printf("[dev %u: ep %02x]: ", xfer->daddr, xfer->ep_addr);
    for(uint32_t i=0; i<xfer->actual_len; i++)
    {
      printf("%02X ", buf[i]);
    }
    printf("\n");

    char s[20];
    snprintf(s, 20, "%02X %02X %02X", buf[1], buf[2], buf[3]); 
    ssd1306_clear(&disp);
    ssd1306_draw_string(&disp, 0, 0, 2, "MIDIHOST");
    ssd1306_draw_string(&disp, 0, 24, 2, s);
    ssd1306_show(&disp);          

    for (uint32_t i = 0; i < 3; i++) 
      note_buf[i] = buf[i+1];
    got_midi = 1;
  }

  // continue to submit transfer, with updated buffer
  // other field remain the same
  xfer->buflen = 64;
  xfer->buffer = buf;

  tuh_edpt_xfer(xfer);
}


//--------------------------------------------------------------------+
// Endpoint Descriptor
//--------------------------------------------------------------------+

void handle_endpoint(uint8_t daddr, tusb_desc_endpoint_t const* desc_ep) {
  if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_IN) {
    // skip if failed to open endpoint
    if (!tuh_edpt_open(daddr, desc_ep)) return;

    in_endpt = desc_ep->bEndpointAddress;

    tuh_xfer_t xfer =
    {
      .daddr       = dev_addr,
      .ep_addr     = in_endpt,
      .buflen      = 64,
      .buffer      = in_buf,
      .complete_cb = data_received,
      .user_data   = (uintptr_t) in_buf, 
    };

    // submit transfer for this EP
    tuh_edpt_xfer(&xfer);

    printf("Listen to [dev %u: ep %02x]\r\n", daddr, desc_ep->bEndpointAddress);
  }  

  if (tu_edpt_dir(desc_ep->bEndpointAddress) == TUSB_DIR_OUT)
  {
    // skip if failed to open endpoint
    if (!tuh_edpt_open(daddr, desc_ep) ) return;

    out_endpt = desc_ep->bEndpointAddress;
    done_startup = 1;  // assumes the out endpoint is the second one enumerated
  }
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

void handle_config_descriptor(uint8_t daddr, tusb_desc_configuration_t const* desc_cfg) {
  printf("Config descriptor\n");

  uint8_t const* p_desc = (uint8_t const*) desc_cfg;
  uint8_t const* desc_end = p_desc + tu_le16toh(desc_cfg->wTotalLength);
 
  while (p_desc < desc_end) {
    switch (tu_desc_type(p_desc)) {
      case TUSB_DESC_CONFIGURATION:           //  0x02
      case TUSB_DESC_INTERFACE:               //  0x04
      case TUSB_DESC_INTERFACE_ASSOCIATION:   //  0x08
      case TUSB_DESC_CS_INTERFACE:            //  0x24
      case TUSB_DESC_CS_ENDPOINT:             //  0x25
        printf("Found descriptor type: %2x\n", tu_desc_type(p_desc));
        break;
      case TUSB_DESC_ENDPOINT:                //  0x05
        printf("Found a USB endpoint \n");
        handle_endpoint(daddr, (tusb_desc_endpoint_t const *) p_desc);
        break;
    }
    p_desc = tu_desc_next(p_desc);
  }
  printf("\n");
}

//--------------------------------------------------------------------+
// Device Descriptor
//--------------------------------------------------------------------+

void handle_device_descriptor(tuh_xfer_t* xfer)
{
  if (xfer->result != XFER_RESULT_SUCCESS)
  {
    printf("Failed to get device descriptor\n");
    return;
  }

  uint8_t const daddr = xfer->daddr;

  printf("Device %u: ID %04x:%04x\r\n", daddr, desc_device.idVendor, desc_device.idProduct);

  uint16_t temp_buf[500];

  // Get configuration descriptor with sync API
  if (tuh_descriptor_get_configuration_sync(daddr, 0, temp_buf, sizeof(temp_buf)) == XFER_RESULT_SUCCESS)
  {
    handle_config_descriptor(daddr, (tusb_desc_configuration_t*) temp_buf);
  }
  else 
    printf("Config descriptor failed\n");
}

//--------------------------------------------------------------------+
// Mount and unmount callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted (configured)
void tuh_mount_cb (uint8_t daddr)
{
  printf("Device attached, address = %d\n", daddr);
  dev_addr = daddr;

  // Get Device Descriptor
  tuh_descriptor_get_device(daddr, &desc_device, 18, handle_device_descriptor, 0);
}

/// Invoked when device is unmounted (bus reset/unplugged)
void tuh_umount_cb(uint8_t daddr)
{
  printf("Device removed, address = %d\n", daddr);
  dev_addr = 0;
  in_endpt = 0;
  out_endpt = 0;
}

//--------------------------------------------------------------------+
// Send config message to the Novation Launchkey 25
//--------------------------------------------------------------------+

void send_startup() {
  tuh_xfer_t xfer3 =
  {
    .daddr       = dev_addr,
    .ep_addr     = out_endpt,
    .buflen      = 8,
    .buffer      = buf_setup,
    .complete_cb = data_sent,
    .user_data   = (uintptr_t) buf_setup, 
    .actual_len  = 8,
  };

  tuh_edpt_xfer(&xfer3);
  printf("Output to [dev %u: ep %02x]\r\n", dev_addr, out_endpt);
}

/*********************************************************************/
// MIDI DEVICE
/*********************************************************************/

void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

void tud_suspend_cb(bool remote_wakeup_en)
{
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}




//--------------------------------------------------------------------+
// Main
//--------------------------------------------------------------------+

int main(void)
{
  board_init();
  printf("MIDI HOST\n");
  setup_display();

  got_midi = 0;

/*
  // Use tuh_configure() to pass pio configuration to the host stack
  // Note: tuh_configure() must be called before
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = 6;
  tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
*/
  // init host stack on configured roothub port
  tuh_init(BOARD_TUH_RHPORT);
  tud_init(BOARD_TUD_RHPORT);

  while (1)
  {
    // tinyusb host task
    if (done_startup == 1) {
      done_startup = 2;
      send_startup();
    };

    tuh_task();
    tud_task();

    if (got_midi) {
      tud_midi_stream_write(0, note_buf, 3);

      got_midi = 0;
    }

    led_blinking_task();
  }

  return 0;
}

