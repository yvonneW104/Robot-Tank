/*
* This file is sending signals from four adc sensors to Xavier.
* Sentinel just send all data without analysising them.
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN
#define SIG_F 7
#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)
                        
static void getValue();
int pid, v5, v4, v6, v7;
char adc_R[100], adc_L[100], adc_F[100], adc_B[100];
int fd7;
char * sensor_fifo7 = "/tmp/my_fifo7";
char * error_Msg = "Error opening file, checking if file exists.\n"; 

/*
* To print all signal information

*/
static void print_siginfo(siginfo_t *si) {
   timer_t *tidp;
   int or;
   tidp = si->si_value.sival_ptr;
   printf("    sival_ptr = %p; ", si->si_value.sival_ptr);
   printf("    *sival_ptr = 0x%lx\n", (long) *tidp);
   or = timer_getoverrun(*tidp);
   if (or == -1)
      errExit("timer_getoverrun");
   else
      printf("    overrun count = %d\n", or);
}

//timer handler
static void handler(int sig, siginfo_t *si, void *uc) {
   /* Note: calling printf() from a signal handler is not
   strictly correct, since printf() is not async-signal-safe;
   see signal(7) */
   getValue(); // get values from adc sensors
   if(kill(pid,SIG_F) != 0){  //print error message
      printf("Can't send msg\n");
      exit(0);
   } 
   if( (fd7 = open(sensor_fifo7, O_WRONLY)) <0){ //print error message
      printf("%s", error_Msg);
      exit(0); //return error code 
   }
   //appends the strings pointed to by adc_B, adc_L, adc_R in order 
   //to end of string pointed to by adc_F
   strcat(adc_F, adc_B); 
   strcat(adc_F, adc_L);
   strcat(adc_F, adc_R);
   write(fd7, adc_F, sizeof(adc_F));
   close(fd7);
   printf("All sent\n"); 
}

/*
* There are four adc distance sensors at each side of the tank.
* This method is to get value from all sensors, which saved as v4, v5, v6, v7.
* The larger the value, the shorter distance between tank and block
*/
static void getValue() {
   FILE* fp;
   fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", "r");
   fgets(adc_L, 100, fp);
   printf("adc_L :%s\n", adc_L);
   v4 = atoi(adc_L);
   fclose(fp);
   fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage3_raw", "r");
   fgets(adc_R, 100, fp);
   printf("adc_R :%s\n", adc_R);
   v5 = atoi(adc_R);
   fclose(fp);
   fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage1_raw", "r");
   fgets(adc_B, 100, fp);
   printf("adc_B :%s\n", adc_B);
   v6 = atoi(adc_B);
   fclose(fp);
   fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage2_raw", "r");
   fgets(adc_F, 100, fp);
   printf("adc_F :%s\n", adc_F);
   v7 = atoi(adc_F);
   fclose(fp);
}

int main(int argc, char *argv[]) {
   timer_t timerid;
   struct sigevent sev;
   struct itimerspec its;
   long long freq_nanosecs;
   sigset_t mask;
   struct sigaction sa;
   FILE* f;
   
   if (argc != 4) {
      fprintf(stderr, "Usage: %s <sleep-secs> <freq-nanosecs>\n",
      argv[0]);
      exit(EXIT_FAILURE);
   }
   pid = atoi(argv[3]);
   printf("pid received is: %d\n\n", pid);

   /* Establish handler for timer signal */
   printf("Establishing handler for signal %d\n", SIG);
   sa.sa_flags = SA_SIGINFO;
   sa.sa_sigaction = handler;
   sigemptyset(&sa.sa_mask);
   if (sigaction(SIG, &sa, NULL) == -1)
      errExit("sigaction");
      
   /* Block timer signal temporarily */
   printf("Blocking signal %d\n", SIG);
   sigemptyset(&mask);
   sigaddset(&mask, SIG);
   if (sigprocmask(SIG_SETMASK, &mask, NULL) == -1)
      errExit("sigprocmask");
   
   /* Create the timer */
   sev.sigev_notify = SIGEV_SIGNAL;
   sev.sigev_signo = SIG;
   sev.sigev_value.sival_ptr = &timerid;
   if (timer_create(CLOCKID, &sev, &timerid) == -1)
      errExit("timer_create");
   printf("timer ID is 0x%lx\n", (long) timerid);
   
   /* Start the timer */
   freq_nanosecs = atoll(argv[2]);
   its.it_value.tv_sec = freq_nanosecs / 1000000000;
   its.it_value.tv_nsec = freq_nanosecs % 1000000000;
   its.it_interval.tv_sec = its.it_value.tv_sec;
   its.it_interval.tv_nsec = its.it_value.tv_nsec;
   
   if (timer_settime(timerid, 0, &its, NULL) == -1)
      errExit("timer_settime");
   
   /* Sleep for a while; meanwhile, the timer may expire
   multiple times */
   printf("Sleeping for %d seconds\n", atoi(argv[1]));
   sleep(atoi(argv[1]));
   
   /* Unlock the timer signal, so that timer notification
   can be delivered */
   printf("Unblocking signal %d\n", SIG);
   if (sigprocmask(SIG_UNBLOCK, &mask, NULL) == -1)
      errExit("sigprocmask");
      
   while(1) {
      //usleep(100000);
   }
   close(f);
   exit(EXIT_SUCCESS);
}