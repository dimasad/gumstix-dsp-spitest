#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#include <netinet/in.h>

#define DATA_LEN 12
#define SPI_SPEED_HZ 500000


struct test_packet {
  uint32_t count;
  uint8_t data[DATA_LEN];
} __attribute__((__packed__));


int fd;
timer_t timer_id;
unsigned calls = 0;
unsigned overruns = 0;
struct test_packet prev_packet, outbound_packet, recv_packet;
struct timespec prev_time, curr_time;


void fail(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}


double tictoc() {
  if (clock_gettime(CLOCK_REALTIME, &curr_time))
    fail("Cannot get time");
  
  double diff = curr_time.tv_sec - prev_time.tv_sec;
  diff += (curr_time.tv_nsec - prev_time.tv_nsec)*1e-9;
  
  prev_time = curr_time;
  return diff;
}


void alarm_handler(int signum, siginfo_t *info, void *context) {
  if (signum != SIGALRM) return;  
  
  calls++;
  overruns += timer_getoverrun(timer_id);
  double period = tictoc();
  
  for (int i=0; i<DATA_LEN; i++)
    outbound_packet.data[i] = rand();
  
  struct spi_ioc_transfer transfer = {
    .tx_buf = (unsigned long)&outbound_packet,
    .rx_buf = (unsigned long)&recv_packet,
    .len = sizeof(struct test_packet),
    .delay_usecs = 0,
    .speed_hz = SPI_SPEED_HZ,
    .bits_per_word = 8,
  };
  
  ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);
  
  int16_t count_diff = ntohs(recv_packet.count) - ntohs(prev_packet.count);
  bool corrupted = false;
  for (int i=0; i<DATA_LEN; i++)
    if (recv_packet.data[i] != prev_packet.data[i]) {
      corrupted = true;
      break;
    }
  
  prev_packet.count = recv_packet.count;
  memcpy(prev_packet.data, outbound_packet.data, DATA_LEN);
  
  printf("%f\t%d\t%d\n", period, count_diff, corrupted);
}


void int_handler(int signum, siginfo_t *info, void *context) {
  if (signum != SIGINT) return;
  
  printf("#calls: %d\toverruns: %d\n", calls, overruns);
  exit(0);
}

int main(int argc, char *argv[]) {
  /**/
  fd = open("/dev/spidev1.1", O_RDWR);
  if (fd < 0)
    fail("Error opening SPI device");
  /**/
  
  uint8_t mode = SPI_MODE_0;
  if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0)
    fail("Error setting spi mode.");
  
  printf("#");
  
  struct sigaction alarm_action;
  alarm_action.sa_sigaction = &alarm_handler;
  alarm_action.sa_flags = 0;
  sigemptyset(&alarm_action.sa_mask);
  sigaddset(&alarm_action.sa_mask, SIGINT);
  sigaction(SIGALRM, &alarm_action, NULL);
  
  struct sigaction int_action;
  int_action.sa_sigaction = &int_handler;
  int_action.sa_flags = 0;
  sigemptyset(&int_action.sa_mask);
  sigaddset(&int_action.sa_mask, SIGALRM);
  sigaction(SIGINT, &int_action, NULL);
  
  if (timer_create(CLOCK_REALTIME, NULL, &timer_id))
    fail("Error creating timer");
  
  tictoc();
  
  struct itimerspec timer_spec;
  timer_spec.it_interval.tv_sec = 0;
  timer_spec.it_value.tv_sec = 0;
  timer_spec.it_interval.tv_nsec = 10000000L;
  timer_spec.it_value.tv_nsec = 10000000L;
  
  if (timer_settime(timer_id, 0, &timer_spec, NULL))
    fail("Error setting timer timeout");
  
  while (1)
    pause();
  
  return 0;
}
