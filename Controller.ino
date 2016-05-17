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

int string = 0;
int waited = 0;
int tuned = 0;

// ----------------------------------------------------------------

void setup() {

  // set motor and direction control pins to outputs
  pinMode(motor_pin[0], OUTPUT);
  pinMode(motor_pin[1], OUTPUT);
  pinMode(motor_pin[2], OUTPUT);
  pinMode(motor_pin[3], OUTPUT);
  pinMode(motor_pin[4], OUTPUT);
  pinMode(motor_pin[5], OUTPUT);
  pinMode(direction_pin, OUTPUT);

  Serial.begin(9600);

  AudioMemory(30);
  note.begin(0.15);

  delay(1000);

  Serial.printf("tuning string %d to between %3.2f and %3.2f Hz\n", string + 1, string_min[string], string_max[string]);

}

// ----------------------------------------------------------------

void loop() {

  // check if a note is available
  if (note.available()) {

    // read frequency
    float f = note.read();

    // read peak voltage
    float p = peak.read() * 1.2;

    // Serial.printf("Note: %3.2f Hz\n", f);
    // Serial.printf("Peak: %3.2f V\n", p);

    if (p > 0.5) {

      Serial.printf("%3.2f\n", f);

      if (!tuned) {

        if (f > string_max[string] && waited == 0) {
          motorRun(string, speed_reverse[string]*(-1));
        }
        else if (f < string_min[string] && waited == 0) {
          motorRun(string, speed_forward[string]);
        }
        else if (waited == 0) {
          motorRun(string, 0);
          delay(500);
          Serial.println("waiting");
          waited = 1;
        }
        else if (waited == 1 && f < string_max[string] && f > string_min[string]) {
          Serial.println("tuned");
          tuned = 1;
        }
        else {
          Serial.println("try again");
          waited = 0;
        }

      }

    } else {
      // too quiet so stop
      motorRun(string, 0);
    }
  } else {
    // no note available so stop
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
  if (speed > 0) {
    duty = map(abs(speed), 0, 100, 0, 255);
    direction = 0;
  }
  else {
    duty = map(abs(speed), 0, 100, 255, 0);
    direction = 1;
  }

  // set all motors off
  for (int i = 0; i < 6; i++) {
    analogWrite(motor_pin[i], direction * 255);
  }

  // set only requested motor on
  analogWrite(motor_pin[motor], duty);

  // set direction pin
  digitalWrite(direction_pin, direction);

}

