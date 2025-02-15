#include <zephyr/kernel.h>

#include "packet_sink.h"
#include "sample_channels.h"

// system clock is 32768Hz
// if taking a sample each 16 ticks
// we get 32768Hz / 16 = 2048Hz sampling rate
#define SAMPLE_PERIOD Z_TIMEOUT_TICKS(16)

static struct k_timer sample_timer;

static void sample_timer_handler(struct k_timer *timer) {
  // this code itself is non-blocking and very fast,
  // also k_work_submit will introduce 0-30us latency variance
  // that is not acceptable, so k_work_submit is avoided
  sample_channels();
  send_packet(sampled_channels_buff, CHANNELS);
}

int main() {
  init_sample_channels();
  init_packet_sink();

  k_timer_init(&sample_timer, sample_timer_handler, NULL);
  k_timer_start(&sample_timer, SAMPLE_PERIOD, SAMPLE_PERIOD);

  while (1) {
    k_sleep(K_FOREVER);
  }

  return 0;
}
