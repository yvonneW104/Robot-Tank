/*
* This file is receving the data from Xavier, and blink the turn light
* according to different command it received.
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#define MAX_BUF 1024
#define LeftLigth 47
#define RightLight 27

FILE *sys, *dirL, *dirR, *valL, *valR;
char buf[MAX_BUF];
int isLeft, isRight, fd_light;
char * sensor_fifo_light = "/tmp/my_fifo_light";

/*
* To request two gpio pins, which connect to two separate LED.
* Set the direction of all gpio pins to "out".
*/
void initial() {
   sys = fopen("/sys/class/gpio/export", "w");
   fprintf(sys, "%d", LeftLigth);
   fflush(sys);
   fprintf(sys, "%d", RightLight);
   fflush(sys);
   dirL = fopen("/sys/class/gpio/gpio47/direction", "w");
   dirR = fopen("/sys/class/gpio/gpio27/direction", "w");
   fprintf(dirL, "%s", "out");
   fprintf(dirR, "%s", "out");
   fflush(dirL);
   fflush(dirR);
   valL = fopen("/sys/class/gpio/gpio47/value", "w");
   valR = fopen("/sys/class/gpio/gpio27/value", "w");
}

/*
* Compare two given char pointer, direction and target.
* If direction is equal to target, change the given gpio value to 1 and 0,
* which is to blink the LED light which connect to gpio.
*/
void blinky(FILE* value) {
  fprintf(value, "%d", 1);
  fflush(value);
  usleep(50000);
  fprintf(value, "%d", 0);
  fflush(value);
  usleep(50000);
}


int main() {   
  initial(); //request gpio pins
  /* create the FIFO (named pipe) */
  mkfifo(sensor_fifo_light, 0666);
  while(1){ //main loop
    fd_light = open(sensor_fifo_light, O_RDONLY);
    read(fd_light, buf, MAX_BUF); //blocking
    if (buf[0] == 'L') {
      blinky(valL);
    } else if (buf[0] == 'R') {
      blinky(valR);
    } else {
      fprintf(valL, "%d", 0);
      fprintf(valR, "%d", 0);
    }
    printf("Read: %s\n", buf);
    close(fd_light);
  }
  /* Close files */
  fclose(sys);
  fclose(dirL);
  fclose(dirR);
  fclose(valL);
  fclose(valR);

  /* remove the FIFO */
  unlink(sensor_fifo_light);
  return 0;
}