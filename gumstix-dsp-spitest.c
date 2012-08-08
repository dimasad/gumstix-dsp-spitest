#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#define PACKET_LEN 10
#define SPI_SPEED_HZ 500000

int fd;
timer_t timer_id;
unsigned calls = 0;
unsigned overruns = 0;
uint8_t data[PACKET_LEN];
struct timespec prev_time, curr_time;


void fail(const char *msg) {
  perror(msg);
  exit(EXIT_FAILURE);
}


double timediff() {
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
  
  struct spi_ioc_transfer transfer = {
    .tx_buf = (unsigned long)data,
    .rx_buf = (unsigned long)data,
    .len = sizeof data,
    .delay_usecs = 0,
    .speed_hz = SPI_SPEED_HZ,
    .bits_per_word = 8,
  };
  
  ioctl(fd, SPI_IOC_MESSAGE(1), &transfer);

  printf("%f\n", timediff());
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

  if (clock_gettime(CLOCK_REALTIME, &prev_time))
    fail("Cannot get time");
  
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
