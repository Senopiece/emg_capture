#include <Arduino.h>

#define PACKET_SIZE 16      // Number of 16-bit values in the packet
#define PACKET_RATE_HZ 7000 // Hz
#define SAMPLE_PIN 4        // Analog pin to read from

uint16_t packet[PACKET_SIZE + 1];
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
    packet[PACKET_SIZE] = 0xFFFF;
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
        for (size_t i = 0; i < PACKET_SIZE; i++)
        {
            packet[i] = count;
        }

        // Send the packet
        Serial.write(data, sizeof(packet));
    }
}