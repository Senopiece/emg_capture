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
adc1_channel_t adc1_channels[] = {ADC1_CHANNEL_7, ADC1_CHANNEL_2, ADC1_CHANNEL_9};
adc2_channel_t adc2_channels[] = {ADC2_CHANNEL_7, ADC2_CHANNEL_0, ADC2_CHANNEL_1};
const size_t adc1_channels_len = sizeof(adc1_channels) / sizeof(adc1_channels[0]);
const size_t adc2_channels_len = sizeof(adc2_channels) / sizeof(adc2_channels[0]);
// esp_adc_cal_characteristics_t adc1_chars;
// esp_adc_cal_characteristics_t adc2_chars;
int raw;

const size_t channels = adc1_channels_len + adc2_channels_len;
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
    {
        adc1_config_width(ADC_WIDTH_BIT_12);
    
        // Configure ADC attenuation
        for (size_t i = 0; i < sizeof(adc1_channels) / sizeof(adc1_channels[0]); i++) {
            adc1_config_channel_atten(adc1_channels[i], ADC_ATTEN);
        }
        for (size_t i = 0; i < sizeof(adc2_channels) / sizeof(adc2_channels[0]); i++) {
            adc2_config_channel_atten(adc2_channels[i], ADC_ATTEN);
        }
    
        // Characterize ADC
        // TODO: maybe apply voltage conversion as this promises to fix nonlinearity of adc
        // esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT_12, 1100, &adc1_chars);
        // esp_adc_cal_characterize(ADC_UNIT_2, ADC_ATTEN, ADC_WIDTH_BIT_12, 1100, &adc2_chars);
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
        {
            for (size_t i = 0; i < adc1_channels_len; i++) {
                packet[i] = adc1_get_raw(adc1_channels[i]);
            }
            for (size_t i = 0; i < adc2_channels_len; i++) {
                adc2_get_raw(adc2_channels[i], ADC_WIDTH_BIT_12, &raw);
                packet[adc1_channels_len + i] = raw;
            }
        }

        #ifdef BENCHMARKME
        // Rewrite for benchmark
        packet[0] = lst - st; // Sample interval - must be near interval_us (mean: inverval_us, variance: typical: +-1us, max: 40us - interestingly variance is always symmetrical meaning no time drift, 40us spikes appear nearly each 10 seconds)
        packet[1] = micros() - lst; // ADC reading (mean: 116.5us, variance: typical: +-1us, max: 44us)
        st = lst;
        #endif

        // Send packet
        Serial.write(data, sizeof(packet));
        ready = false;
    }
}