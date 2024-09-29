#pragma once
#include <ADS1256.h>
#include <Arduino.h>
#include <SPI.h>
#define MOSFET_PIN 15
// #define SELFCAL_AT_START true

// Structure for calibration variables
struct calibrationData {
     double voltageOffset;
     float voltageGainCurrent;
     float voltageGainPG16;
     float voltageGainPG8;
     float voltageGainPG4;
     float voltageGainPG2;
     double currentOffset5m;
     double currentOffset100m;
     double currentGain5m;
     double currentGain100m;
     double staticVoltageDrop;  // per ampere, in volts
};

struct actualData {
    volatile double voltage;
    volatile double current;
    volatile double power;
    volatile double energy;
    volatile double mAh;
    volatile float peakCurrent;
    volatile float peakVoltage;
    volatile float peakPower;
    volatile float minVoltage = 99;
    volatile float minCurrent = 99;
    volatile float minPower = 999;
    volatile bool peakChanged = true;
    volatile double dynamicVoltageDrop;
    bool measureOutputVoltage = false;
    // Add more variables if needed
};

extern volatile unsigned long startTime;
extern volatile unsigned long elapsedTime;
extern bool forceUse5mShunt;

struct ADCCalibration {
    long OFC0;
    long OFC1;
    long OFC2;
    long FSC0;
    long FSC1;
    long FSC2;
    // Add more calibration variables if needed
};

extern double alarmVoltage;
extern bool lowVoltageAlarm;
extern volatile bool ADCInitialized;
extern bool hold;
extern bool ADCPaused;
extern int inputChannel;
extern long rawConversion;  // 24-bit raw value
extern float voltageValue;  // human-readable floating point value
extern int differentialChannels[4];
extern calibrationData calibration;
extern actualData readData;
extern actualData holdData;
extern ADCCalibration PGA4Cal, PGA8Cal, PGA16Cal, PGA32Cal, PGA64Cal;

#define VREF 2.4955f
extern ADS1256 A;

void initADC();
void readCalRegisters(ADCCalibration* data);
void writeCalRegisters(ADCCalibration data);
void readADC();
void selfCalibrate();

void startStopwatch();
void pauseStopwatch();
void resumeStopwatch();
void resetStopwatch();
void updateStopwatch();