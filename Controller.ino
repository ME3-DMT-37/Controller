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

int motor_pin[] = {3, 4, 5, 6, 10, 9};
int direction_pin = 11;

// ----------------------------------------------------------------

float string_min[] = {81.94, 109.37, 145.98, 194.87, 245.52, 327.73};
float string_max[] = {82.89, 110.64, 147.68, 197.14, 248.37, 331.54};

float speed_forward[] = {65, 50, 80, 90, 50, 90};
float speed_reverse[] = {30, 40, 40, 50, 30, 70};

// ----------------------------------------------------------------

bool waited = false;
bool tuned = false;

// ----------------------------------------------------------------

int string = 1;

// ----------------------------------------------------------------

void setup() {

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

  // log string number and target frequency range
  Serial.printf("string: %d\n", string + 1);
  Serial.printf("target: % 3.2f to % 3.2f Hz\n\n", string_min[string], string_max[string]);

  AudioMemory(30);
  note.begin(0.15);

}

// ----------------------------------------------------------------

void loop() {

  // check note availability
  if (note.available()) {

    // read frequency and peak voltage
    float f = note.read();
    float p = peak.read() * 1.2;

    // check tuned flag and peak voltage
    if ((!tuned) && (p > 0.5)) {

      // log note and peak voltage
      Serial.printf("note: %3.2f Hz (%3.2f V)\n", f, p);

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

          // log status
          Serial.println("wait");

          // stop motor
          motorRun(string, 0);

          // delay for settling
          delay(500);

          // raise waited flag
          waited = true;

        } else {

          // log status
          Serial.println("tuned");

          // reset waited flag
          waited = false;

          // raise tuned flag
          tuned = true;

        }

      }

    } else {
      motorRun(string, 0);
    }

  } else {
    motorRun(string, 0);
  }

  delay(100);

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

