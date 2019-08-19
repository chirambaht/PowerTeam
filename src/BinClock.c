/*
 * BinClock.c
 * Jarrod Olivier
 * Modified for EEE3095S/3096S by Keegan Crankshaw
 * August 2019
 * 
 * MGDTAT002 CHRHUM001
 * 13 August 2019
*/

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <stdio.h> //For printf functions
#include <stdlib.h> // For system functions

#include "BinClock.h"
#include "CurrentTime.h"

//Global variables
int hours, mins, secs;
long lastInterruptTime = 0; //Used for button debounce
int RTC; //Holds the RTC instance

int HH,MM,SS;

void int_to_bin_digit(unsigned int in, int count, int* out)
{
    /* assert: count <= sizeof(int)*CHAR_BIT */
    unsigned int mask = 1U << (count-1);
    int i;
    for (i = 0; i < count; i++) {
        out[i] = (in & mask) ? 1 : 0;
        in <<= 1;
    }
}


void initGPIO(void){
	/*
	 * Sets GPIO using wiringPi pins. see pinout.xyz for specific wiringPi pins
	 * You can also use "gpio readall" in the command line to get the pins
	 * Note: wiringPi does not use GPIO or board pin numbers (unless specifically set to that mode)
	 */
	printf("Setting up\n");
	if (wiringPiSetup() == -1){ //This is the default mode. If you want to change pinouts, be aware
		printf("setting up wiring pi failed");
    }
	RTC = wiringPiI2CSetup(RTCAddr); //Set up the RTC

	//Set up the LEDS
	for(int i = 0; i < sizeof(LEDS)/sizeof(LEDS[0]); i++){
	    pinMode(LEDS[i], OUTPUT);
		digitalWrite(LEDS[i], LOW);
	}

	//Set Up the Seconds LED for PWM
	//Write your logic here
	pinMode(SECS, PWM_OUTPUT);

	printf("LEDS done\n");

	//Set up the Buttons
	pinMode(BTNS[0], INPUT);
	pullUpDnControl(BTNS[0], PUD_UP);
	if (wiringPiISR(BTNS[0], INT_EDGE_FALLING,&minInc) != 0){
			printf("registering isr for button %x failed \n", BTNS[0]);
	}
	
	pinMode(BTNS[1], INPUT);
	pullUpDnControl(BTNS[1], PUD_UP);
	if (wiringPiISR(BTNS[1], INT_EDGE_FALLING,&hourInc) != 0){
			printf("registering isr for button %x failed \n", BTNS[1]);
	}

	//Attach interrupts to Buttons
	
	//Write your logic here

	printf("BTNS done\n");
	printf("Setup done\n");
}


/*
 * The main function
 * This function is called, and calls all relevant functions we've written
 */
 
int main(void){

	initGPIO();

	//Set random time (3:54PM)
	//You can comment this file out later
	wiringPiI2CWriteReg8(RTC, HOUR, 0x13+TIMEZONE);
	wiringPiI2CWriteReg8(RTC, MIN, 0x04);
	wiringPiI2CWriteReg8(RTC, SEC, 0x00);
	// Repeat this until we shut down

	for (;;){
		//Fetch the time from the RTC
		mins = decCompensation(wiringPiI2CReadReg8(RTC, MIN));
		hours = decCompensation(wiringPiI2CReadReg8(RTC, HOUR));
		secs = decCompensation(wiringPiI2CReadReg8(RTC, SEC));
		
		//Function calls to toggle LEDs
		
		
		secs++;
		
		if (secs == 60){
			secs = 0;
			mins++;
		}
		
		if (mins == 60){
			mins = 0;
			hours++;
		}
		
		if (hours == 20){
			hours = 0;
		}
		
		secPWM(secs);
		lightHours(hours);
		lightMins(mins);
		secPWM(secs);
		
		// Print out the time we have stored on our RTC
		printf("The current time is: %x:%x:%x\n", hours, mins, secs);

		//using a delay to make our program "less CPU hungry"
		delay(1000); //milliseconds
	}
	return 0;
}

/*
 * Change the hour format to 12 hours
 */
int hFormat(int hours){
	/*formats to 12h*/
	if (hours >= 24){
		hours = 0;
	}
	else if (hours > 12){
		hours -= 12;
	}
	return (int)hours;
}

/*
 * Turns on corresponding LED's for hours
 */
void lightHours(int units){
	// Write your logic to light up the hour LEDs here	
	int hourArray[4];
	int_to_bin_digit(units, 4, hourArray);
	
	for (int i = 0; i < 4; i++){
		digitalWrite(LEDS[i], hourArray[i]);
	}
}

/*
 * Turn on the Minute LEDs
 */
void lightMins(int units){
	//Write your logic to light up the minute LEDs here
	int minuteArray[6];
	int_to_bin_digit(units, 6, minuteArray);
	
	for (int i = 0; i < 6; i++){
		digitalWrite(LEDS[i+4], minuteArray[i]);
	}
	
	
}

/*
 * PWM on the Seconds LED
 * The LED should have 60 brightness levels
 * The LED should be "off" at 0 seconds, and fully bright at 59 seconds
 */
void secPWM(int units){
	// Write your logic here
	double dutyCycle = (1024/59) * units;
	pwmWrite(SECS, dutyCycle);
}

/*
 * hexCompensation
 * This function may not be necessary if you use bit-shifting rather than decimal checking for writing out time values
 */
int hexCompensation(int units){
	/*Convert HEX or BCD value to DEC where 0x45 == 0d45 
	  This was created as the lighXXX functions which determine what GPIO pin to set HIGH/LOW
	  perform operations which work in base10 and not base16 (incorrect logic) 
	*/
	int unitsU = units%0x10;

	if (units >= 0x50){
		units = 50 + unitsU;
	}
	else if (units >= 0x40){
		units = 40 + unitsU;
	}
	else if (units >= 0x30){
		units = 30 + unitsU;
	}
	else if (units >= 0x20){
		units = 20 + unitsU;
	}
	else if (units >= 0x10){
		units = 10 + unitsU;
	}
	return units;
}


/*
 * decCompensation
 * This function "undoes" hexCompensation in order to write the correct base 16 value through I2C
 */
int decCompensation(int units){
	int unitsU = units%10;

	if (units >= 50){
		units = 0x50 + unitsU;
	}
	else if (units >= 40){
		units = 0x40 + unitsU;
	}
	else if (units >= 30){
		units = 0x30 + unitsU;
	}
	else if (units >= 20){
		units = 0x20 + unitsU;
	}
	else if (units >= 10){
		units = 0x10 + unitsU;
	}
	return units;
}


/*
 * hourInc
 * Fetch the hour value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 23 hours in a day
 * Software Debouncing should be used
 */
void hourInc(void){
	//Debounce
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
// 		printf("Interrupt 1 triggered, %x\n", hours);
		//Fetch RTC Time
		int temp = wiringPiI2CReadReg8(RTC, HOUR);
		//Increase hours by 1, ensuring not to overflow
		hours = decCompensation(temp);
		hours++;
		if (hours == 24){
			hours = 0;
		}
		temp = hexCompensation(hours);
		//Write hours back to the RTC
		wiringPiI2CWriteReg8(RTC, HOUR, temp);
	}
	lastInterruptTime = interruptTime;
}

/* 
 * minInc
 * Fetch the minute value off the RTC, increase it by 1, and write back
 * Be sure to cater for there only being 60 minutes in an hour
 * Software Debouncing should be used
 */
void minInc(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		//Fetch RTC Time
		int temp = wiringPiI2CReadReg8(RTC, MIN);
		//Increase minutes by 1, ensuring not to overflow
		mins = decCompensation(temp);
		mins++;
		if (mins == 60){
			hourInc();
			mins = 0;
		}
		temp = hexCompensation(mins);
		//Write minutes back to the RTC
		wiringPiI2CWriteReg8(RTC, MIN, temp);
	}
	lastInterruptTime = interruptTime;
}

//This interrupt will fetch current time from another script and write it to the clock registers
//This functions will toggle a flag that is checked in main
void toggleTime(void){
	long interruptTime = millis();

	if (interruptTime - lastInterruptTime>200){
		HH = getHours();
		MM = getMins();
		SS = getSecs();

		HH = hFormat(HH);
		HH = decCompensation(HH);
		wiringPiI2CWriteReg8(RTC, HOUR, HH);

		MM = decCompensation(MM);
		wiringPiI2CWriteReg8(RTC, MIN, MM);

		SS = decCompensation(SS);
		wiringPiI2CWriteReg8(RTC, SEC, 0b10000000+SS);

	}
	lastInterruptTime = interruptTime;
}
