#include <Arduino.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <soc/adc_channel.h>

// NOTE: a channel value is read using internal 12-bit adc - each channel has two bytes in a packet
#define PACKET_RATE_HZ 2000 // Hz
#define ADC_ATTEN ADC_ATTEN_DB_12 // 3.3V max

const uint32_t interval_us = 1000000 / PACKET_RATE_HZ; // Interval in us (500us)

hw_timer_t *timer = NULL;
volatile bool ready = false;

// ADC configuration
adc1_channel_t adc1_pins[] = {ADC1_CHANNEL_3, ADC1_CHANNEL_4, ADC1_CHANNEL_5, ADC1_CHANNEL_7, ADC1_CHANNEL_2, ADC1_CHANNEL_8};
const size_t channels = sizeof(adc1_pins) / sizeof(adc1_pins[0]);
int raw;

uint16_t packet[channels + 1];
const uint8_t *data = reinterpret_cast<const uint8_t *>(&packet);

#ifdef BENCHMARKME
uint32_t st, lst;
#endif

void IRAM_ATTR onTimer()
{
    ready = true;
}

void setup()
{
    // USBCDC
    Serial.begin(BAUDRATE);
    Serial.setTxTimeoutMs(1);  // Lower timeout for quick transmission

    // Initialize ADC
    adc1_config_width(ADC_WIDTH_BIT_12);
    for (size_t i = 0; i < channels; i++) {
        adc1_config_channel_atten(adc1_pins[i], ADC_ATTEN);
    }

    // packet delimiter
    packet[channels] = 0xFFFF;

    // Setup Timer (ID=0, Divider=80 for 1µs resolution, Mode=autoreload, No ISR)
    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, interval_us, true);
    timerAlarmEnable(timer);

    #ifdef BENCHMARKME
    st = micros();
    #endif
}


void loop()
{
    if (ready)
    {
        #ifdef BENCHMARKME
        lst = micros();
        #endif

        // Read ADC values
        for (size_t i = 0; i < channels; i++) {
            packet[i] = adc1_get_raw(adc1_pins[i]);
        }

        #ifdef BENCHMARKME
        // Rewrite for benchmark
        packet[0] = lst - st; // Sample interval - must be near interval_us (mean: inverval_us, variance: typical: 0us, max: -20us +20us - spikes occur about once per 10 seconds, interestingly variance is always symmetrical meaning no time drift)
        packet[1] = micros() - lst; // ADC reading (mean: 144.5us, variance: typical: 0us, max: -0us +40us, +40us spikes occur about each four seconds)
        st = lst;
        #endif

        // Send packet
        Serial.write(data, sizeof(packet));
        ready = false;
    }
}