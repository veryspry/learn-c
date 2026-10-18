#include <mach/host_info.h>
#include <mach/mach.h>
#include <mach/machine.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "./ring-buffer.h"

#define STATIC_BUFFER_SIZE 512

static size_t weights_len = STATIC_BUFFER_SIZE;

static host_cpu_load_info_data_t *cpu_info_buf[STATIC_BUFFER_SIZE];

void read_ticks(host_cpu_load_info_data_t *info) {
  mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

  host_statistics(
    mach_host_self(),
    HOST_CPU_LOAD_INFO,
    (host_info_t)info,
    &count
  );
}

void clear_line() {
    printf("\x1b[1F"); // Move to beginning of previous line
    printf("\x1b[2K"); // Clear entire line
}

void start_poll() {
  int read = 0;
  int write = 0;


  printf("CPU Usage\n-------------\n\n");

  while (1) {
    host_cpu_load_info_data_t *data;
    data = malloc(sizeof(host_cpu_load_info_data_t));

    read_ticks(data);

    cpu_info_buf[write] = data;

    double user = 0;
    double sys = 0;
    double nice = 0;
    double idle = 0;

    for (int i = read; i <= write; i++) {
      user += cpu_info_buf[i]->cpu_ticks[CPU_STATE_USER];
      sys += cpu_info_buf[i]->cpu_ticks[CPU_STATE_SYSTEM];
      nice += cpu_info_buf[i]->cpu_ticks[CPU_STATE_NICE];
      idle += cpu_info_buf[i]->cpu_ticks[CPU_STATE_IDLE];
    }

    double total = user + sys + nice + idle;

    clear_line();

    printf(
      "CPU total: %.1f%% | user: %.1f%% | system: %.1f%% | nice: %.1f%% | idle: %.1f%% \n",
      100.0 * (user + sys+ nice) / total,
      100.0 * user / total,
      100.0 * sys / total,
      100.0 * nice / total,
      100.0 * idle / total
    );

    if (write + 1 < (int)weights_len) {
      write++;
    } else {
      write = 0;
    }

    usleep(500000);
  }
}

int main() {
  start_poll();

  return 0;
}
