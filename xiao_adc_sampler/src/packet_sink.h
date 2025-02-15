#pragma once

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>

typedef uint16_t PacketElem;

const struct device *cdc_dev;
const PacketElem _packet_delimiter = 0xFFFF;

void init_packet_sink() { cdc_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console)); }

// this code is not blocking, but may cause data loss
// NOTE: logging may interfere
void send_packet(PacketElem values[], uint8_t len) {
  // assert no 0xFFFF in values
  uart_fifo_fill(cdc_dev, (uint8_t *)values, len * sizeof(PacketElem));
  uart_fifo_fill(cdc_dev, (uint8_t *)&_packet_delimiter,
                 sizeof(_packet_delimiter));
}