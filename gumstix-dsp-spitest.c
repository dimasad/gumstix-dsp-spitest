#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>


timer_t timer_id;
unsigned calls = 0;
unsigned overruns = 0;


void alarm_handler(int signum, siginfo_t *info, void *context) {
  if (signum != SIGALRM) return;  

  calls++;
  overruns += timer_getoverrun(timer_id);
}

void int_handler(int signum, siginfo_t *info, void *context) {
  if (signum != SIGINT) return;
  
  printf("calls: %d\toverruns: %d\n", calls, overruns);
  exit(0);
}

void fail(const char *msg) {
  perror(msg); 
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
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
  
  struct itimerspec timer_spec;
  timer_spec.it_interval.tv_sec = 0;
  timer_spec.it_value.tv_sec = 0;
  timer_spec.it_interval.tv_nsec = 50000000L;
  timer_spec.it_value.tv_nsec = 50000000L;
  if (timer_settime(timer_id, 0, &timer_spec, NULL))
    fail("Error setting timeout");

  while (1)
    pause();
  
  return 0;
}
