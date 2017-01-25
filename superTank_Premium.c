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
#include <string.h>
#include <math.h>
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
char * command;
int dL, dR, fd_light, vF, vL, vR, vB， directionCode;
char * sensor_fifo_light = "/tmp/my_fifo_light";
char adc_R[100], adc_L[100], adc_F[100], adc_B[100];

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
	fprintf(period1, "%d", 1000000);
	fflush(period1);
	duty_cycle1 = fopen("/sys/devices/ocp.2/pwm_test_P9_42.12/duty", "w");
	fprintf(duty_cycle1, "%d", 1000000);
	fflush(duty_cycle1);
	enable1 = fopen("/sys/devices/ocp.2/pwm_test_P9_42.12/run", "w");
	fprintf(enable1, "%d", 1);
	fflush(enable1);
	period2 = fopen("/sys/devices/ocp.2/pwm_test_P9_28.17/period", "w");
	fprintf(period2, "%d", 1000000);
	fflush(period2);
	duty_cycle2 = fopen("/sys/devices/ocp.2/pwm_test_P9_28.17/duty", "w");
	fprintf(duty_cycle2, "%d", 1000000);
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

void getSensorValue() {
	FILE* fp;
	fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage0_raw", "r");
	fgets(adc_L, 100, fp);
	printf("adc_L :%s\n", adc_L);
	vL = atoi(adc_L);
	fclose(fp);
	fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage3_raw", "r");
	fgets(adc_R, 100, fp);
	printf("adc_R :%s\n", adc_R);
	vR = atoi(adc_R);
	fclose(fp);
	fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage1_raw", "r");
	fgets(adc_B, 100, fp);
	printf("adc_B :%s\n", adc_B);
	vB = atoi(adc_B);
	fclose(fp);
	fp = fopen("/sys/bus/iio/devices/iio:device0/in_voltage2_raw", "r");
	fgets(adc_F, 100, fp);
	printf("adc_F :%s\n", adc_F);
	vF = atoi(adc_F);
	fclose(fp);
	if (vF > 3500) {
		command = "B";
		directionCode = Back;
	} else if (vL > 3500 && vR < vL) {
		command = "R";
		directionCode = Right;
	} else if (vR > 3500) {
		command = "L";
		directionCode = Left;
	} else if (vB > 3500) {
		command = "F";
		directionCode = Front;
	}
}

int main() {   
    initial(); //request pins
    FILE* value[5] = {val, val1, val2, val3, val4};
    int fd, j, vL, vR, iL, iR, L, R;
    int oL = 0, oR = 0;
    char * ttyO4 = "/dev/ttyO4";
    //char buf[MAX_BUF];
    /* create the FIFO (named pipe) */
    while(1){ //main loop
        fd = open(ttyO4, O_RDONLY);
        read(fd, buf, MAX_BUF);//blocking
        printf("buf received: %s\n", buf);
        close(fd);
        if(buf[0] == 'r' || buf[0] == 's') {
            if(buf[0] == 'r') {
                vL = 0;
                vR = 0;
            } else if(buf[1] == ',' && (buf[3] == ',' || buf[4] == ',' || buf[5] == ',')) {
                char *p;
                p = strtok(buf, ",");
                p = strtok(NULL, ",");
                vL = atoi(p);
                p = strtok(NULL, ",");
                vR = atoi(p);
                oL = vL;
                oR = vR;
            } else {
            	vL = oL;
            	vR = oR;
            }
            printf("vL %d\n", vL);
            printf("vR %d\n", vR);

            L = (int) log((double) abs(vL)); //(double) abs(vL)
            R = (int) log((double) abs(vR));

            iL = 1000000 - L * 217145;
            iR = 1000000 - R * 217145;

            if(vL == 0 && vR ==0) {
                directionCode = Stop;
                command = "S";
            } else if(vL >= 0 && vR >= 0) {
                directionCode = Front;
                command = "F";
            } else if(vL >= 0 && vR < 0) {
                directionCode = Right;
                command = "R";
            } else if(vL < 0 && vR >= 0) {
                directionCode = Left;
                command = "L";
            } else {
                directionCode = Back;
                command = "B";
            }

            getSensorValue();

            fd_light = open(sensor_fifo_light, O_WRONLY | O_NONBLOCK);
            write(fd_light, command, sizeof(command)); 
            printf("write dierction: %s\n", command);
            close(fd_light);
        
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
            fprintf(duty_cycle1, "%d", iR);
            fprintf(duty_cycle2, "%d", iL);

            //close(fd); 
            fflush(val);
            fflush(val1);
            fflush(val2);
            fflush(val3);
            fflush(val4);
            fflush(duty_cycle1);
            fflush(duty_cycle2);
        } else {
        	usleep(10);
        }
        

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
    return 0;
}