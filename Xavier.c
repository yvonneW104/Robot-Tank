/*
* This file is receving data from sentinel (four adc sensors).
* Determine the direction of tank according to the different value 
* the sensor detected. And then write the command to two files 
* h_bridge and turnLight (h_bridge is to drive the motor, and 
* turnLight is to blink turn light)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#define SIG_F 7
#define MAX_BUF 1024

int fd, vF, vB, vL, vR, fd7, fd_light;
char * sensor_fifo7 = "/tmp/my_fifo7";
char * sensor_fifo = "/tmp/my_fifo";
char * sensor_fifo_light = "/tmp/my_fifo_light";
char * oldCommand = "S"; // set initial command to stop
char * command;
char * error_Msg = "Error opening file, checking if file exists.\n";
char buf[MAX_BUF];

int direction();
int light();

//timer handler
void sig_handlerF(int signo) {
   fd7 = open(sensor_fifo7, O_RDONLY);
   read(fd7, buf, MAX_BUF); //blocking
   printf("On the Front: %s\n", buf);
   char *p;
   /*
   * break the buf by '\n' and convert them into int 
   * and assign to value Left, Right, Front and Back
   */
   p = strtok(buf, "\n");
   vF = atoi(p);
   p = strtok(NULL, "\n");
   vB = atoi(p);
   p = strtok(NULL, "\n");
   vL = atoi(p);
   p = strtok(NULL, "\n");
   vR = atoi(p);
   printf("%d\n", vF);
   printf("%d\n", vB);
   printf("%d\n", vL);
   printf("%d\n", vR);
   close(fd7);
}

int main(void) {  
   mkfifo(sensor_fifo7, 0666);
   while(1) { //main loop
      if (signal(SIG_F, sig_handlerF) == SIG_ERR)
         printf("\ncan't catch SIGINT\n");
      direction(); //to get the direction, and write to sensor_fifo
      light(); //to get the direction, and write to sensor_fifo_light
      oldCommand = command; //update the new command as the "old" one
      usleep(1);
   }
   unlink(sensor_fifo7);
   return 0;
}

/*
* This method is mainly to determine the direction to go.
* With the four values detected by the adc distance sensors, to set 
* five directions for the tank, which are stop, front, back, left, and right.
* 
* The special case is that the tank will stop if there are blocks aroung it.
* If there are blocks in front, at left, and at right, the tank will go back.
* If there are blocks in front, at rigth, the tank will turn left.
* If there are blocks in front, at left, the tank will turn rigth.
* If none of above happened, the tank just go forward.
*/
int direction() {
   if(vF>3500 && vB>3500 && vL>3500 && vR>3500) {
      command = "B";
   } else if(vF>2500 && vL>2500 && vR>2500) {
      command = "B";
   } else if(vF > 2500) {
      if((vL > vR) && vL > 2000) {
         command = "R";
      } else {
         command = "L";
      }
   } else if((vF>2000 && vR>2000) || vR > 3500) { 
      command = "L";
   } else if((vF>2000 && vL>2000) || vL > 3500) { 
      command = "R";
   } else {
      command = "F";
   }
   
   /*
   * To write the command only if the command is changed,
   * in sensor_fifo, which is send to "h_bridge"
   */
   if(strcmp(command, oldCommand) != 0) {
      if( (fd = open(sensor_fifo, O_WRONLY | O_NONBLOCK)) <0){
        printf("%s", error_Msg);
        return -1; //return error code 
      }
      write(fd, command, sizeof(command)); 
      printf("write new command %s\n", command);
      close(fd);
   }
   return 0;
}

/*
* This method is mainly to blink turn light.
* 
* To write the command only if the command is changed,
* in sensor_fifo_light, which is send to "turnLight"
*/
int light() {
   if(strcmp(command, oldCommand) != 0) {
      if( (fd = open(sensor_fifo_light, O_WRONLY | O_NONBLOCK)) <0){
        printf("%s", error_Msg);
        return -1; //return error code 
      }
      write(fd_light, command, sizeof(command)); 
      printf("write new command %s\n", command);
      close(fd_light);
   }
   return 0;
}


