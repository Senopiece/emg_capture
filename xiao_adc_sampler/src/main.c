#include <zephyr/kernel.h>

#include "packet_sink.h"
#include "sample_channels.h"

#define SAMPLING_FREQUENCY 2048
#define SAMPLE_PERIOD                                                          \
  Z_TIMEOUT_TICKS(sys_clock_hw_cycles_per_sec() / SAMPLING_FREQUENCY)

static struct k_timer sample_timer;
static struct k_work sample_work;

static void sample_work_handler(struct k_work *work) {
  sample_channels();
  send_packet(sampled_channels_buff, CHANNELS);
}

static void sample_timer_handler(struct k_timer *timer) {
  k_work_submit(&sample_work);
}

int main() {
  init_packet_sink();
  init_sample_channels();

  k_work_init(&sample_work, sample_work_handler);
  k_timer_init(&sample_timer, sample_timer_handler, NULL);
  k_timer_start(&sample_timer, SAMPLE_PERIOD, SAMPLE_PERIOD);

  while (1) {
    k_sleep(K_FOREVER);
  }

  return 0;
}
