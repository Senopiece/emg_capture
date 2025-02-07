#include <Arduino.h>

// TODO: maybe oversample the adc to get a more accurate reading

// NOTE: a channel value is read using internal 12-bit adc - each channel has two bytes in a packet
#define CHANNELS 2          // Number of channels a packet is carrying
#define PACKET_RATE_HZ 2000 // Hz

const uint8_t pin[CHANNELS] = {3, 11}; // Analog pins to read from

uint16_t packet[CHANNELS + 1];
const uint8_t *data = reinterpret_cast<const uint8_t *>(&packet);

uint32_t last_time = 0;
const uint32_t interval_us = 1000000 / PACKET_RATE_HZ; // Interval in microseconds

void setup()
{
    // USBCDC
    Serial.begin(BAUDRATE);

    // Initialize analog pins
    for (size_t i = 0; i < CHANNELS; i++) pinMode(pin[i], INPUT);

    // prepare packet end
    packet[CHANNELS] = 0xFFFF;
}

void loop()
{
    uint32_t current_time = micros();

    // Check if it's time to send the next packet
    if (current_time - last_time >= interval_us)
    {
        // Read channels and populate packet
        for (size_t i = 0; i < CHANNELS; i++) packet[i] = analogRead(pin[i]);

        // Send the packet
        // assert data is not OxFFFF - passes since adc is only 12 bit - the rest of the bits are 0
        Serial.write(data, sizeof(packet));

        // Update time
        last_time = current_time;
    }
}