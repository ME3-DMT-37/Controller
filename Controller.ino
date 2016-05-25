#include <Audio.h>
#include <EEPROM.h>
#include <SD.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <Wire.h>

// ----------------------------------------------------------------

#define FORWARD 0
#define REVERSE 1

// ----------------------------------------------------------------

float f;
float p;

int led_pin = 13;

int motor_pin[] = {3, 4, 5, 6, 10, 9};

int direction_pin = 11;

bool string_detuned[] = {true, true, true, true, true, true};
bool string_calibrated[] = {true, true, true, true, true, true};
bool string_tuned[] = {true, true, true, true, true, true};

int silent = 0;

bool waited = false;

float string_low[] = {81.94, 109.37, 145.98, 194.87, 245.52, 327.73};
float string_high[] = {82.89, 110.64, 147.68, 197.14, 248.37, 331.54};

int speed_forward[] = {100, 100, 100, 100, 100, 100};
int speed_reverse[] = {100, 100, 100, 100, 100, 100};

int string = 1;

const int memory = 5;

float history[memory];

int speed = 10; // will only work once!

int iteration = 0;

// ----------------------------------------------------------------

AudioInputAnalog          adc;
AudioAnalyzeNoteFrequency note;
AudioAnalyzePeak          peak;
AudioConnection           patchCord(adc, note);
AudioConnection           patchCord2(adc, peak);

// ----------------------------------------------------------------

void setup() {

  pinMode(led_pin, OUTPUT);

  // set up motors
  motorSetup();

  // open serial port
  Serial.begin(9600);

  // delay for serial port initialisation
  delay(1000);

  // allocate memory to audio library
  AudioMemory(30);
  note.begin(0.15);

  string_detuned[string] = true;
  string_calibrated[string] = true;
  string_tuned[string] = false;

}

// ----------------------------------------------------------------

void loop() {

  if (sample()) {

    if (!string_detuned[string]) {
      detune(string, REVERSE);
    } else if (!string_calibrated[string]) {
      calibrate(string, FORWARD);
    } else if (!string_tuned[string]) {
      tune(string);
    }

    silent = 0;

  } else {

    motorRun(string, 0);

    silent = silent + 1;

    if (silent == 5) {
      Serial.println("strum again");
    }

  }

  delay(100);

}

// ----------------------------------------------------------------

bool sample() {

  // check note availability
  if (note.available()) {

    // read frequency and peak voltage
    f = note.read();
    p = peak.read() * 1.2;

    // remove mains interference and check peak voltage then raise flag
    if ((f > 55) && (p > 0.3)) {
      return true;
    } else {
      return false;
    }

  } else {
    return false;
  }

}

// ----------------------------------------------------------------

void detune(int string, int direction) {

  digitalWrite(led_pin, LOW);

  // log note and peak voltage
  Serial.printf("detuning: %3.2f Hz (%3.2f V)\n", f, p);

  if ((direction == FORWARD) && (f < string_high[string])) {

    // tigthten string
    motorRun(string, 100);

  } else if ((direction == REVERSE) && (f > string_low[string])) {

    // loosen string
    motorRun(string, -100);

  } else {

    // stop motor
    motorRun(string, 0);

    // raise success flag
    string_detuned[string] = true;

    digitalWrite(led_pin, HIGH);

    // log status
    Serial.printf("detuning: done\n\n");

    delay(1000);

  }

}

// ----------------------------------------------------------------

void calibrate(int string, int direction) {

  float total = 0;
  float average = 0;

  digitalWrite(led_pin, LOW);

  // store frequency reading
  history[iteration] = f;

  Serial.printf("calibrating: %3.2f Hz (%d)\n", f, speed);

  // check for enough values to proceed
  if (iteration >= (memory - 1)) {

    // calculate total
    for (int i = 0; i < memory; i++) {
      total = total + history[i];
    }

    // calculate average
    average = total / float(memory);

    if (direction == FORWARD) {

      if (f < string_high[string]) {

        // check for stalling (no change in frequency)
        if ((abs(f - average) < 0.5) && (speed <= 100)) {

          // increment speed
          speed = speed + 5;

          // set motor speed
          motorRun(string, speed);

          // log status
          Serial.printf("calibrating: stalled\n\n");

        } else {

          // log status
          Serial.printf("calibrating: continue\n\n");

        }

      } else {

        // set motor off
        motorRun(string, 0);

        // save speed
        speed_forward[string] = speed;

        // update EEPROM
        EEPROM.update(string, speed);

        // raise calibrated flag
        string_calibrated[string] = true;

        digitalWrite(led_pin, HIGH);

        // log frequency, averge frequency, and speed and status
        Serial.printf("calibrating: done\n\n");

        // wait
        delay(1000);

      }

    }

    if (direction == REVERSE) {

      // check for movement (small change in frequency)
      if ((abs(f - average) < 0.5) && (speed <= 100)) {

        // increment speed
        speed = speed + 5;

        // set motor speed
        motorRun(string, speed * (-1));

        // log status
        Serial.printf("calibrating: stalled\n\n");

      } else {

        // set motor off
        motorRun(string, 0);

        // save speed
        speed_reverse[string] = speed;

        // update EEPROM
        EEPROM.update(string + 6, speed);

        // raise calibrated flag
        string_calibrated[string] = true;

        digitalWrite(led_pin, HIGH);

        // log frequency, averge frequency, and speed and status
        Serial.printf("calibrating: done\n\n");

        // wait
        delay(1000);

      }

    }

    // reset iteration
    iteration = 0;

  } else {

    // increment iteration
    iteration = iteration + 1; //check if this resets iterations when guitar voltage drops below threshold voltage

  }

}

// ----------------------------------------------------------------

void tune(int string) {

  digitalWrite(led_pin, LOW);

  // log note and peak voltage
  Serial.printf("tuning: %3.2f Hz (%3.2f V)\n", f, p);

  if (f > string_high[string]) {

    // loosen string (over-tuned)
    motorRun(string, speed_reverse[string] * (-1));

    // lower waited flag
    waited = false;

  } else if (f < string_low[string]) {

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
      Serial.printf("tuning: waiting\n\n");

    } else {

      // reset waited flag
      waited = false;

      // raise tuned flag
      string_tuned[string] = true;

      // set LED on
      digitalWrite(led_pin, HIGH);

      // log status
      Serial.printf("tuning: done\n");

      delay(1000);

    }

  }

}

// ----------------------------------------------------------------

void motorSetup() {

  // set motor control pins to outputs
  for (int i = 0; i < 6; i++) {
    pinMode(motor_pin[i], OUTPUT);
  }

  // set direction control pin to outputs
  pinMode(direction_pin, OUTPUT);

  // recover speeds from memory
  for (int i = 0; i < 6; i++) {

    speed_forward[i] = EEPROM.read(i);
    speed_reverse[i] = EEPROM.read(i + 6);

    Serial.printf("string %d: +%d / -%d\n", i + 1, speed_forward[i], speed_reverse[i]);

  }

}

// ----------------------------------------------------------------

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
