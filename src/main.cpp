#include <Arduino.h>

// TODO: maybe oversample the adc to get a more accurate reading

// NOTE: a channel value is read using internal 12-bit adc - each channel has two bytes in a packet
#define CHANNELS 6          // Number of channels a packet is carrying
#define PACKET_RATE_HZ 2000 // Hz

const uint8_t pin[CHANNELS] = {18, 8, 3, 9, 10, 11}; // Analog pins to read from

uint16_t packet[CHANNELS + 1];
const uint8_t *data = reinterpret_cast<const uint8_t *>(&packet);

const uint32_t interval_us = 1000000 / PACKET_RATE_HZ; // Interval in microseconds

hw_timer_t *timer = NULL;
volatile bool ready = false;

void IRAM_ATTR onTimer()
{
    ready = true;
}

void setup()
{
    // USBCDC
    Serial.begin(BAUDRATE);
    Serial.setTxTimeoutMs(1);  // Lower timeout for quick transmission

    // Initialize analog pins
    for (size_t i = 0; i < CHANNELS; i++) pinMode(pin[i], INPUT);

    // prepare packet end
    packet[CHANNELS] = 0xFFFF;

    // Setup Timer (ID=0, Divider=80 for 1µs resolution, Mode=autoreload, No ISR)
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, interval_us, true);
    timerAlarmEnable(timer);
}

void loop()
{
    if (ready)
    {
        for (size_t i = 0; i < CHANNELS; i++) packet[i] = analogRead(pin[i]);        
        Serial.write(data, sizeof(packet));
        ready = false;
    }
}
