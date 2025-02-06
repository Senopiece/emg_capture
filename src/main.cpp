#include <Arduino.h>

// NOTE: using internal 12-bit adc - each channel has two bytes in a packet
#define CHANNELS 16         // Number of channels a packet is carrying
#define PACKET_RATE_HZ 7000 // Hz
#define SAMPLE_PIN 4        // Analog pin to read from

uint16_t packet[CHANNELS + 1];
const uint8_t *data = reinterpret_cast<const uint8_t *>(&packet);

uint32_t last_time = 0;
const uint32_t interval_us = 1000000 / PACKET_RATE_HZ; // Interval in microseconds

uint32_t count = 0;

void setup()
{
    // USBCDC
    Serial.begin(BAUDRATE);

    // Initialize analog pin
    // pinMode(SAMPLE_PIN, INPUT);

    // prepare packet end
    packet[CHANNELS] = 0xFFFF;
}

void loop()
{
    uint32_t current_time = micros();

    // Check if it's time to send the next packet
    if (current_time - last_time >= interval_us)
    {
        last_time = current_time;

        // Read analog value
        // uint16_t value = analogRead(SAMPLE_PIN);

        // update count
        count = (count + 1) % 4096;

        // Populate the packet with dummy data (e.g., incrementing values for testing)
        for (size_t i = 0; i < CHANNELS; i++)
        {
            packet[i] = count;
        }

        // Send the packet
        // assert data is not OxFFFF
        Serial.write(data, sizeof(packet));
    }
}