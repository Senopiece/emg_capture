#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>

#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
#include <hal/nrf_saadc.h>
#endif

#define CHANNELS 6
#define ADC_RESOLUTION 12

static const struct device *adc_dev = DEVICE_DT_GET(DT_NODELABEL(adc));

static struct adc_channel_cfg adc_channels[CHANNELS];
static struct adc_sequence adc_seq;
static uint16_t sampled_channels_buff[CHANNELS];

void init_sample_channels(void) {
  for (int i = 0; i < CHANNELS; i++) {
    adc_channels[i].channel_id = i;
    adc_channels[i].differential = 0;

    /* Use maximum acquisition time to reduce noise */
    adc_channels[i].acquisition_time = ADC_ACQ_TIME_DEFAULT;
    adc_channels[i].reference = ADC_REF_INTERNAL; // 0.6 V internal reference

    /* Use gain of 1/6 to allow 3.6V input (3.6V * 1/6 ≈ 0.6V) */
    adc_channels[i].gain = ADC_GAIN_1_6;

#ifdef CONFIG_ADC_CONFIGURABLE_INPUTS
    /* Map each channel to an ADC input pin. For nRF, assume channels 0-5 map
    to
     * AIN0-AIN5 */
    adc_channels[i].input_positive = NRF_SAADC_INPUT_AIN0 + i;
#endif

    adc_channel_setup(adc_dev, &adc_channels[i]);
  }

  adc_seq.options = NULL;
  adc_seq.channels = BIT_MASK(CHANNELS);
  adc_seq.buffer = sampled_channels_buff;
  adc_seq.buffer_size = sizeof(sampled_channels_buff);
  adc_seq.resolution = ADC_RESOLUTION;
  adc_seq.oversampling = 0; // not allowed with scan mode
  adc_seq.calibrate = false;
}

void sample_channels(void) { adc_read(adc_dev, &adc_seq); }
