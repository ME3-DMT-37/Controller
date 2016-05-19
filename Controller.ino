#include <Audio.h>
#include <SD.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <Wire.h>

// ----------------------------------------------------------------

AudioInputAnalog          adc;
AudioAnalyzeNoteFrequency note;
AudioAnalyzePeak          peak;
AudioConnection           patchCord(adc, note);
AudioConnection           patchCord2(adc, peak);

// ----------------------------------------------------------------

int led_pin = 13;
int motor_pin[] = {3, 4, 5, 6, 10, 9};
int direction_pin = 11;

// ----------------------------------------------------------------

float string_min[] = {81.94, 109.37, 145.98, 194.87, 245.52, 327.73};
float string_max[] = {82.89, 110.64, 147.68, 197.14, 248.37, 331.54};

float speed_forward[] = {65, 50, 80, 60, 50, 90};
float speed_reverse[] = {30, 40, 40, 30, 30, 70};

// ----------------------------------------------------------------

void setup() {

  pinMode(led_pin, OUTPUT);

  // set motor control pins to outputs
  for (int i = 0; i < 6; i++) {
    pinMode(motor_pin[i], OUTPUT);
  }

  // set direction control pin to outputs
  pinMode(direction_pin, OUTPUT);

  // open serial port
  Serial.begin(9600);

  // delay for serial port initialisation
  delay(1000);

  AudioMemory(30);
  note.begin(0.15);

}

// ================================================================

float f;
float p;

bool loosened = false;
bool calibrated = false;
bool waited = false;
bool tuned = false;

// ----------------------------------------------------------------

void loop() {

  int string = 1;

  // check note availability
  if (note.available()) {

    // read frequency and peak voltage
    f = note.read();
    p = peak.read() * 1.2;

    // remove mains interference and check peak voltage
    if ((f > 55) && (p > 0.4)) {

      if (!loosened) {
        loosen(string);
      } else if (!calibrated) {
        calibrate(string);
      } else if (!tuned) {
        tune(string);
      }

    } else {
      motorRun(string, 0);
    }

  } else {
    motorRun(string, 0);
  }

  delay(100);

}

// ================================================================

void loosen(int string) {

  // log note and peak voltage
  Serial.printf("loosening: %3.2f Hz (%3.2f V)\n", f, p);

  if (f > string_min[string]) {

    // loosen string (over tuned)
    motorRun(string, -100);

  } else {

    // stop motor
    motorRun(string, 0);

    // raise success flag
    loosened = true;

    // log status
    Serial.printf("loosening: done\n\n");

  }

}

// ================================================================

const int memory = 5;

float history[memory];

int speed = 5; // will only work once!

int iteration = 0;

// ----------------------------------------------------------------

void calibrate(int string) {

  float total = 0;
  float average = 0;

  if ((f < string_max[string]) || (speed >= 100)) {

    // store frequency reading
    history[iteration] = f;

    // check for enough values to proceed
    if (iteration > (memory - 1)) {

      // calculate total
      for (int i = 0; i < memory; i++) {
        total = total + history[i];
      }

      // calculate average
      average = total / float(memory);

      // check for stalling (small change in frequency)
      if (abs(f - average) < 0.5) {

        // increment speed
        speed = speed + 5;

      }

      // reset iteration
      iteration = 0;

    } else {
      
      // increment iteration
      iteration = iteration + 1;
      
    }

    // set motor speed
    motorRun(string, speed);

    // log frequency, averge frequency, and speed
    Serial.printf("calibrating: %3.2f Hz (%d)\n", f, speed);

  } else {

    // set motor off
    motorRun(string, 0);

    // raise calibrated flag
    calibrated = true;

    // log frequency, averge frequency, and speed and status
    Serial.printf("calibrating: %3.2f Hz (%d)\n", f, speed);
    Serial.printf("calibrating: done\n\n");

  }

}

// ================================================================

void tune(int string) {

  // log note and peak voltage
  Serial.printf("tuning: %3.2f Hz (%3.2f V)\n", f, p);

  if (f > string_max[string]) {

    // loosen string (over-tuned)
    motorRun(string, speed_reverse[string] * (-1));

    // lower waited flag
    waited = false;

  } else if (f < string_min[string]) {

    // tighten string (under-tuned)
    motorRun(string, speed_forward[string]);

    // lower waited flag
    waited = false;

  } else {

    if (!waited) {

      // set motor off
      motorRun(string, 0);

      // delay for settling
      delay(500);

      // raise waited flag
      waited = true;

      // log status
      Serial.printf("tuning: waiting\n");

    } else {

      // reset waited flag
      waited = false;

      // raise tuned flag
      tuned = true;

      // set LED on
      digitalWrite(led_pin, HIGH);

      // log status
      Serial.printf("tuning: done\n");

    }

  }

}

// ================================================================

void motorRun(int motor, int speed) {

  // define duty cycle and direction variables
  int duty = 0;
  int direction = 0;

  // map power percentage to duty cycle
  if (speed >= 0) {
    duty = map(abs(speed), 0, 100, 0, 255);
    direction = 0;
  } else {
    duty = map(abs(speed), 0, 100, 255, 0);
    direction = 1;
  }

  // set all motors off
  for (int i = 0; i < 6; i++) {
    analogWrite(motor_pin[i], direction * 255);
  }

  // set requested motor on
  analogWrite(motor_pin[motor], duty);

  // set direction pin
  digitalWrite(direction_pin, direction);

}

