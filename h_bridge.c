/*
* This file is receving the data from Xavier, and drive the motor to different 
* directions according to the command it received. 
*/

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#define MAX_BUF 1024
#define AIN2 46
#define AIN1 26
#define STBY 45
#define BIN1 65
#define BIN2 44
#define stopLight 60

int gpio[5] = {AIN2, AIN1, STBY, BIN1, BIN2};
int Stop = 0b00000, Back = 0b10110, Left = 0b10101, Right = 0b01110, Front = 0b01101;
FILE *sys, *dir, *dir1, *dir2, *dir3, *dir4, *dir_stopLight, *val, *val1, *val2, 
	*val3, *val4, *val_stopLight, *period1, *period2, *duty_cycle1, *enable1,
      *duty_cycle2, *enable2, *dirLeftLight, *valLeftLigth; 
char buf[MAX_BUF];

/*
* To request the gpio pins and PWM pins. Set the direction of all gpio pins to "out".
*/
void initial() {
	sys = fopen("/sys/class/gpio/export", "w");
	fprintf(sys, "%d", AIN1);
	fflush(sys);
	fprintf(sys, "%d", AIN2);
	fflush(sys);
	fprintf(sys, "%d", STBY);
	fflush(sys);
	fprintf(sys, "%d", BIN1);
	fflush(sys);
	fprintf(sys, "%d", BIN2);
	fflush(sys);
	fprintf(sys, "%d", stopLight);
	fflush(sys);

	dir = fopen("/sys/class/gpio/gpio46/direction", "w");
	dir1 = fopen("/sys/class/gpio/gpio26/direction", "w");
	dir2 = fopen("/sys/class/gpio/gpio45/direction", "w");
	dir3 = fopen("/sys/class/gpio/gpio65/direction", "w");
	dir4 = fopen("/sys/class/gpio/gpio44/direction", "w");
	dir_stopLight = fopen("/sys/class/gpio/gpio60/direction", "w");

	fprintf(dir, "%s", "out");
	fprintf(dir1, "%s", "out");
	fprintf(dir2, "%s", "out");
	fprintf(dir3, "%s", "out");
	fprintf(dir4, "%s", "out");
	fprintf(dir_stopLight, "%s", "out");
	fflush(dir);
	fflush(dir1);
	fflush(dir2);
	fflush(dir3);
	fflush(dir4);
	fflush(dir_stopLight);
	/*
	* As for PWM we use P9 pin 42 and P9 pin 28, which P9_42 controls the right motor,
	* and P9_28 controls the left motor
	*/
	period1 = fopen("/sys/devices/ocp.2/pwm_test_P9_42.12/period", "w");
	fprintf(period1, "%d", 500000);
	fflush(period1);
	duty_cycle1 = fopen("/sys/devices/ocp.2/pwm_test_P9_42.12/duty", "w");
	fprintf(duty_cycle1, "%d", 110000);
	fflush(duty_cycle1);
	enable1 = fopen("/sys/devices/ocp.2/pwm_test_P9_42.12/run", "w");
	fprintf(enable1, "%d", 1);
	fflush(enable1);
	period2 = fopen("/sys/devices/ocp.2/pwm_test_P9_28.17/period", "w");
	fprintf(period2, "%d", 500000);
	fflush(period2);
	duty_cycle2 = fopen("/sys/devices/ocp.2/pwm_test_P9_28.17/duty", "w");
	fprintf(duty_cycle2, "%d", 150000);
	fflush(duty_cycle2);
	enable2 = fopen("/sys/devices/ocp.2/pwm_test_P9_28.17/run", "w");
	fprintf(enable2, "%d", 1);
	fflush(enable2);
	
	val = fopen("/sys/class/gpio/gpio46/value", "w");
	val1 = fopen("/sys/class/gpio/gpio26/value", "w");
	val2 = fopen("/sys/class/gpio/gpio45/value", "w");
	val3 = fopen("/sys/class/gpio/gpio65/value", "w");
	val4 = fopen("/sys/class/gpio/gpio44/value", "w");
	val_stopLight = fopen("/sys/class/gpio/gpio60/value", "w");
}

int main() {   
    initial(); //request pins
    FILE* value[5] = {val, val1, val2, val3, val4};
    int fd;
    char * sensor_fifo = "/tmp/my_fifo"; 
    char buf[MAX_BUF];
    /* create the FIFO (named pipe) */
    mkfifo(sensor_fifo, 0666);
    while(1){ //main loop
        fd = open(sensor_fifo, O_RDONLY);
        read(fd, buf, MAX_BUF); //blocking
        printf("Received: %s\n", buf);
        int directionCode, j;
        if (buf[0] == 'S') {
            directionCode = Stop;
        } else if (buf[0] == 'B') {
            directionCode = Back;
        } else if (buf[0] == 'L') {
            directionCode = Left;
        } else if (buf[0] == 'R') {
            directionCode = Right;
        } else {
            directionCode = Front;
        }
        /*
		* If going back, the back light is on.
        */
        if (directionCode == Back) {
        	fprintf(val_stopLight, "%d", 1);
        } else {
        	fprintf(val_stopLight, "%d", 0);// turn off if not going back
        }
        fflush(val_stopLight);

        //assign values to each gpio pin with given directionCode
        for (j = 0; j < 5; j++) {
            fprintf(value[j], "%d", (directionCode >> j) & 1);
        } 

        close(fd); 
        fflush(val);
        fflush(val1);
        fflush(val2);
        fflush(val3);
        fflush(val4);

    }
    //close files
    fclose(sys);
	fclose(dir);
	fclose(dir1);
	fclose(dir2);
	fclose(dir3);
	fclose(dir4);
	fclose(dir_stopLight);
	fclose(val);
	fclose(val1);
	fclose(val2);
	fclose(val3);
	fclose(val4);
	fclose(val_stopLight);
	fclose(period1);
	fclose(duty_cycle1);
	fclose(enable1);
	fclose(period2);
	fclose(duty_cycle2);
	fclose(enable2);
    /* remove the FIFO */
    unlink(sensor_fifo);
    return 0;
}