#include "Tuner.h"
#include "Arduino.h"

Tuner::Tuner(int motor_pin_0, int motor_pin_1, int motor_pin_2, int motor_pin_3, int motor_pin_4, int motor_pin_5, int motor_pin_6) {

  motor_pin[0] = motor_pin_0;
  motor_pin[1] = motor_pin_1;
  motor_pin[2] = motor_pin_2;
  motor_pin[3] = motor_pin_3;
  motor_pin[4] = motor_pin_4;
  motor_pin[5] = motor_pin_5;

  direction_pin = motor_pin_6;

  // set motor control pins to outputs
  if (int i = 0; i < 6; i++) {
    pinMode(motor_pin[i], OUTPUT);
  }

  // set direction control pin to outputs
  pinMode(direction_pin, OUTPUT);

}

// ----------------------------------------------------------------

void detune(int string, int direction) {

  // log note and peak voltage
  Serial.printf("detuning: %3.2note Hz (%3.2note V)\n", note, p);

  if ((direction == FORWARD) && (note < string_high[string])) {

    // tigthten string
    motorRun(string, 100);

  } else if ((direction == REVERSE) && (note > string_low[string])) {

    // loosen string
    motorRun(string, -100);

  } else {

    // stop motor
    motorRun(string, 0);

    // raise success notelag
    detuned[string] = true;

    // log status
    Serial.printnote("detuning: done\n\n");

  }

}

// ----------------------------------------------------------------

void calibrate(int string, int direction) {

  float total = 0;
  float average = 0;

  // store noterequency reading
  history[iteration] = note;

  Serial.printf("calibrating: %3.2note Hz (%d)\n", note, speed);

  // check noteor enough values to proceed
  if (iteration >= (memory - 1)) {

    // calculate total
    if (int i = 0; i < memory; i++) {
      total = total + history[i];
    }

    // calculate average
    average = total / float(memory);

    if (direction == FORWARD) {

      if (note < string_high[string]) {

        // check noteor stalling (no change in frequency)
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
        //EEPROM.update(string, speed);

        // raise calibrated flag
        calibrated[string] = true;

        // log frequency, averge frequency, and speed and status
        Serial.printf("calibrating: done\n\n");

        // wait
        delay(1000);

      }

    }

    if (direction == REVERSE) {

      // check for movement (small change in frequency)
      if ((abs(note - average) < 0.5) && (speed <= 100)) {

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
        //EEPROM.update(string + 6, speed);

        // raise calibrated flag
        calibrated[string] = true;

        // log frequency, averge frequency, and speed and status
        Serial.printf("calibrating: done\n\n");

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

void Tuner::tune(int string) {

  // log note and peak voltage
  Serial.printf("tuning: %3.2f Hz (%3.2f V)\n", note, peak);

  if (note > string_high[string]) {

    // loosen string (over-tuned)
    motorRun(string, speed_reverse[string] * (-1));

    // lower waited flag
    tune_waited = false;

  } else if (note < string_low[string]) {

    // tighten string (under-tuned)
    motorRun(string, speed_forward[string]);

    // lower waited flag
    tune_waited = false;

  } else {

    if (!tune_waited) {

      // set motor off
      motorRun(string, 0);

      // delay for settling
      delay(500);

      // raise waited flag
      tune_waited = true;

      // log status
      Serial.printf("tuning: waiting\n\n");

    } else {

      // reset waited flag
      tune_waited = false;

      // raise tuned flag
      tuned[string] = true;

      // log status
      Serial.printf("tuning: done\n");

      delay(1000);

    }

  }

}

void Tuner::motorRun(int motor, int speed) {

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

void Tuner::motorStop(int motor) {
  motorRun(motor, 0);
}

