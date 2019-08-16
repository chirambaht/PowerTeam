#ifndef BINCLOCK_H
#define BINCLOCK_H

//Some reading (if you want)
//https://stackoverflow.com/questions/1674032/static-const-vs-define-vs-enum

// Function definitions
int hFormat(int hours); //Done
void lightHours(int units); //Tatenda
void lightMins(int units); //Tatenda
int hexCompensation(int units); //Done
int decCompensation(int units); //Done
void initGPIO(void); //Tatenda
void secPWM(int units); //Tatenda
void hourInc(void); //Humphrey -- done?
void minInc(void); //Humphrey -- done?
void toggleTime(void); //Done
// Main function //Humphrey

// define constants
const char RTCAddr = 0x6f;
const char SEC = 0x00; // see register table in datasheet
const char MIN = 0x01;
const char HOUR = 0x02;
const char TIMEZONE = 2; // +02H00 (RSA)

// define pins
const int LEDS[] = {8,10,16,18,22,24,26,32,36,38}; //H0-H4, M0-M5
const int SECS = 12;
const int BTNS[] = {13,15}; // B0, B1


#endif
