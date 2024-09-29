#include <Arduino.h>
#include <SPI.h>
#include <adc.h>
#include <button.h>
#define MOSFET_PIN 15
// #define RANDOMIZE_DATA  // for debugging
#define SELFCAL_AT_START

bool forceUse5mShunt = true;

volatile unsigned long startTime = 0;
volatile unsigned long elapsedTime = 0;

bool lowVoltageAlarm = false;
double alarmVoltage = 0;
volatile bool ADCInitialized = false;
bool hold = false;
bool ADCPaused = true;
bool firstRead = true;
int inputChannel = 0;
long rawConversion = 0;                                                  // 24-bit raw value
float voltageValue = 0;                                                  // human-readable floating point value
int differentialChannels[4] = {DIFF_0_1, DIFF_2_3, DIFF_4_5, DIFF_6_7};  // Array to store the differential channels
byte alarmCount = 0;
calibrationData calibration;
actualData readData;
actualData holdData;
ADCCalibration PGA2Cal, PGA4Cal, PGA8Cal, PGA16Cal, PGA32Cal, PGA64Cal;

// #define VREF 2.4939f
ADS1256 A(5, 0, 0, 6, VREF);  // DRDY, RESET, SYNC(PDWN), CS, VREF(float). RESET & PDWN not used.

void initADC() {
    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW);
    SPI.begin(9, 7, 8, 6);
    SPI.setFrequency(SPI_CLOCK_DIV2);
    Serial.println("Initializing ADS1256");

    A.InitializeADC();
    A.setPGA(PGA_4);
    A.setMUX(DIFF_0_1);
    A.setDRATE(DRATE_30SPS);
    A.setAutoCal(true);
    delay(100);
    A.sendDirectCommand(SELFCAL);
    A.sendDirectCommand(ACAL_ENABLED);

#ifdef SELFCAL_AT_START
    selfCalibrate();
#endif
    // Serial.end();
    calibration.voltageGainPG16 = 22.2709302116;
    calibration.voltageGainPG8 = 22.2750785762;
    calibration.voltageGainPG4 = 22.2737692935;
    calibration.voltageGainPG2 = 22.2737692935;  // NOT CALIBRATED!
    calibration.voltageOffset = 0.0;
    calibration.currentGain5m = 197.992142363;
    calibration.currentGain100m = 9.96020965673;
    calibration.staticVoltageDrop = 1 / 1000;  // per ampere, in volts
    ADCInitialized = true;
}

void readCalRegisters(ADCCalibration* thisData) {
    thisData->OFC0 = A.readRegister(OFC0_REG);
    thisData->OFC1 = A.readRegister(OFC1_REG);
    thisData->OFC2 = A.readRegister(OFC2_REG);
    thisData->FSC0 = A.readRegister(FSC0_REG);
    thisData->FSC1 = A.readRegister(FSC1_REG);
    thisData->FSC2 = A.readRegister(FSC2_REG);
}

void writeCalRegisters(ADCCalibration thisData) {
    A.writeRegister(OFC0_REG, thisData.OFC0);
    A.writeRegister(OFC1_REG, thisData.OFC1);
    A.writeRegister(OFC2_REG, thisData.OFC2);
    A.writeRegister(FSC0_REG, thisData.FSC0);
    A.writeRegister(FSC1_REG, thisData.FSC1);
    A.writeRegister(FSC2_REG, thisData.FSC2);
}

void readADC() {
    updateStopwatch();
    if (firstRead == true) {
        // readData.startMillis = 0;
        firstRead = false;
    }
    // ----------Current Reading------------
    if (forceUse5mShunt == true) digitalWrite(MOSFET_PIN, LOW);
    if (digitalRead(MOSFET_PIN) == LOW) {
        // Serial.println("using 5m ohm");
        A.setMUX(DIFF_4_5);
        if (readData.current < 15.2) {
            A.setPGA(PGA_64);  // 78.125mV or 15.625A
            writeCalRegisters(PGA64Cal);
        } else {
            A.setPGA(PGA_32);  // 156.25mV or 31.25A
            writeCalRegisters(PGA32Cal);
        }
        readData.current = A.convertToVoltage(A.readSingle());
        readData.dynamicVoltageDrop = readData.current;
        readData.current *= calibration.currentGain5m;
#ifdef RANDOMIZE_DATA
        // Simulate data update
        // readData.current = random(0, 50) / 10.0;
        readData.current = 1.0;
#endif
        // Serial.println(readData.current, 9);
        if (readData.current < 0.00001 && readData.current > -1) readData.current = 0.0;
        // if (readData.current < 2.0)
        // digitalWrite(MOSFET_PIN, HIGH);
    } else {
        // Serial.println("using 100m ohm");
        A.setMUX(DIFF_2_3);

        if (readData.current < 0.74) {
            A.setPGA(PGA_64);  // 78.125mV or 0.78125A
            writeCalRegisters(PGA64Cal);
        } else if (readData.current < 1.5) {
            A.setPGA(PGA_32);  // 156.25mV or 1.5626A
            writeCalRegisters(PGA32Cal);
        } else {
            A.setPGA(PGA_16);  // 312.5mV or 3.125A
            writeCalRegisters(PGA16Cal);
        }
        readData.current = A.convertToVoltage(A.readSingle());
        readData.dynamicVoltageDrop = readData.current;
        readData.current *= calibration.currentGain100m;
#ifdef RANDOMIZE_DATA
        // Simulate data update
        // readData.current = random(0, 50) / 10.0;
        readData.current = 1.0;
#endif
        // Serial.println(readData.current, 9);
        if (readData.current < 0.0001 && readData.current > -1) readData.current = 0.0;
        if (readData.current >= 2.0)
            digitalWrite(MOSFET_PIN, LOW);
    }

    // ----------Voltage Reading------------
    A.setMUX(DIFF_6_7);
    if (readData.voltage < 6.5) {
        calibration.voltageGainCurrent = calibration.voltageGainPG16;
        // Serial.print("PGA 16: ");
        A.setPGA(PGA_16);  // 312.5mV or 6.875V
        writeCalRegisters(PGA64Cal);
    } else if (readData.voltage < 13.5) {
        calibration.voltageGainCurrent = calibration.voltageGainPG8;
        // Serial.print("PGA 8: ");
        A.setPGA(PGA_8);  // 625mV or 13.75V
        writeCalRegisters(PGA8Cal);
    } else if (readData.voltage < 13.5) {
        calibration.voltageGainCurrent = calibration.voltageGainPG4;
        // Serial.print("PGA 4: ");
        A.setPGA(PGA_4);  // 1.25 or 27.5
        writeCalRegisters(PGA4Cal);
    } else {
        calibration.voltageGainCurrent = calibration.voltageGainPG2;
        // Serial.print("PGA 2: ");
        A.setPGA(PGA_2);  // 2.5V or 55V
        writeCalRegisters(PGA2Cal);
    }
    readData.voltage = A.convertToVoltage(A.readSingle());
    // Serial.println(readData.voltage, 9);
    readData.voltage *= calibration.voltageGainCurrent;
    readData.voltage -= calibration.voltageOffset;
    if (readData.measureOutputVoltage == true) {
        double totalVoltageDrop = readData.dynamicVoltageDrop + (calibration.staticVoltageDrop * readData.current);
        readData.voltage -= totalVoltageDrop;
        Serial.println("Voltage drop: " + String(totalVoltageDrop));
    }
#ifdef RANDOMIZE_DATA
    // Simulate data update
    // readData.voltage = random(220, 240) / 10.0;
    readData.voltage = 20.0;
#endif
    if (readData.voltage < 0.05 && readData.voltage > -1) readData.voltage = 0.0;
    // Serial.println(readData.voltage, 9);

    if (lowVoltageAlarm && readData.voltage <= alarmVoltage) {
        if (alarmCount < 15) {
            tone(BUZZER_PIN, TONE_FREQUENCY * 1.25, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY * 0.75, TONE_DURATION);
            alarmCount++;
        } else if (alarmCount < 30) {
            tone(BUZZER_PIN, TONE_FREQUENCY * 1.25, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY * 0.75, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY * 1.25, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY * 0.75, TONE_DURATION);
            alarmCount++;
        } else {
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
            tone(BUZZER_PIN, TONE_FREQUENCY, TONE_DURATION);
        }
    } else {
        alarmCount = 0;
    }

    readData.power = readData.voltage * readData.current;
    if (readData.peakVoltage < readData.voltage) {
        readData.peakVoltage = readData.voltage;
        readData.peakChanged = true;
    }
    if (readData.peakCurrent < readData.current) {
        readData.peakCurrent = readData.current;
        readData.peakChanged = true;
    }
    if (readData.peakPower < readData.power) {
        readData.peakPower = readData.power;
        readData.peakChanged = true;
    }
    if(readData.minVoltage > readData.voltage && readData.voltage > 0.05) readData.minVoltage = readData.voltage;
    if(readData.minCurrent > readData.current && readData.current > 0.05) readData.minCurrent = readData.current;
    if(readData.minPower > readData.power && readData.power > 0.05) readData.minPower = readData.power;
    // if(readData.minVoltage != 99) Serial.println("min voltage updated: " + String(readData.minVoltage));
    // A.stopConversion();
}

void selfCalibrate() {
    delay(100);
    readCalRegisters(&PGA4Cal);
    Serial.println(String(PGA4Cal.OFC0) + String(PGA4Cal.OFC1) + String(PGA4Cal.OFC2));
    Serial.println(String(PGA4Cal.FSC0) + String(PGA4Cal.FSC1) + String(PGA4Cal.FSC2));

    A.setPGA(PGA_2);
    delay(100);
    A.sendDirectCommand(SELFCAL);
    delay(100);
    readCalRegisters(&PGA2Cal);
    Serial.println(String(PGA2Cal.OFC0) + String(PGA2Cal.OFC1) + String(PGA2Cal.OFC2));
    Serial.println(String(PGA2Cal.FSC0) + String(PGA2Cal.FSC1) + String(PGA2Cal.FSC2));

    A.setPGA(PGA_8);
    delay(100);
    A.sendDirectCommand(SELFCAL);
    delay(100);
    readCalRegisters(&PGA8Cal);
    Serial.println(String(PGA8Cal.OFC0) + String(PGA8Cal.OFC1) + String(PGA8Cal.OFC2));
    Serial.println(String(PGA8Cal.FSC0) + String(PGA8Cal.FSC1) + String(PGA8Cal.FSC2));

    A.setPGA(PGA_16);
    delay(100);
    A.sendDirectCommand(SELFCAL);
    delay(100);
    readCalRegisters(&PGA16Cal);
    Serial.println(String(PGA16Cal.OFC0) + String(PGA16Cal.OFC1) + String(PGA16Cal.OFC2));
    Serial.println(String(PGA16Cal.FSC0) + String(PGA16Cal.FSC1) + String(PGA16Cal.FSC2));

    A.setPGA(PGA_32);
    delay(100);
    A.sendDirectCommand(SELFCAL);
    delay(100);
    readCalRegisters(&PGA32Cal);
    Serial.println(String(PGA32Cal.OFC0) + String(PGA32Cal.OFC1) + String(PGA32Cal.OFC2));
    Serial.println(String(PGA32Cal.FSC0) + String(PGA32Cal.FSC1) + String(PGA32Cal.FSC2));

    A.setPGA(PGA_64);
    delay(100);
    A.sendDirectCommand(SELFCAL);
    delay(100);
    readCalRegisters(&PGA64Cal);
    Serial.println(String(PGA64Cal.OFC0) + String(PGA64Cal.OFC1) + String(PGA64Cal.OFC2));
    Serial.println(String(PGA64Cal.FSC0) + String(PGA64Cal.FSC1) + String(PGA64Cal.FSC2));
}

volatile unsigned long pausedTime = 0;
void startStopwatch() {
    startTime = millis();
    //   hold = false;
    // Serial.println("Start");
}

void pauseStopwatch() {
    //   hold = true;
    pausedTime += millis() - startTime;
    // Serial.print("Paused: ");
    // Serial.println(pausedTime);
}

void resumeStopwatch() {
    //   hold = false;
    // Correctly calculate the new startTime based on previous elapsed time
    startTime = millis() - elapsedTime - pausedTime;
    // Serial.print("Resumed: ");
    // Serial.println(startTime);
}

void resetStopwatch() {
    startTime = millis();
    elapsedTime = 0;
    pausedTime = 0;
    hold = false;
    // Serial.println("Reset");
}

void updateStopwatch() {
    if (!hold) {
        elapsedTime = millis() - startTime - pausedTime;
        // Serial.print("Elapsed: ");
        // Serial.println(elapsedTime);

        // // Additional debug information
        // Serial.print("millis(): ");
        // Serial.print(millis());
        // Serial.print(", startTime: ");
        // Serial.print(startTime);
        // Serial.print(", pausedTime: ");
        // Serial.println(pausedTime);
    }
}
