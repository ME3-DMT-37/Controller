#ifndef Tuner_h
#define Tuner_h

#include "Arduino.h"

#define FORWARD 0
#define REVERSE 1

class Tuner {

  public:

    Tuner(int motor_pin_0, int motor_pin_1, int motor_pin_2, int motor_pin_3, int motor_pin_4, int motor_pin_5, int direction_pin);

    void detune(int string, int direction);
    void calibrate(int string, int direction);
    void tune(int string);
    void motorRun(int motor, int speed);
    void motorStop(int motor);

    float note;
    float peak;

    float string_low[6] = {81.94, 109.37, 145.98, 194.87, 245.52, 327.73};
    float string_high[6] = {82.89, 110.64, 147.68, 197.14, 248.37, 331.54};

    bool detuned[6]    = {true, true, true, true, true, true};
    bool calibrated[6] = {true, true, true, true, true, true};
    bool tuned[6]      = {true, true, true, true, true, true};

    int speed_forward[6] = {100, 100, 100, 100, 100, 100};
    int speed_reverse[6] = {100, 100, 100, 100, 100, 100};

  private:

    bool tune_waited = false;

    float calibrate_history[5];
    int calibrate_memory = 5;
    int calibrate_speed = 10; // will only work once!
    int calibrate_iteration = 0;

    int motor_pin[6];
    int direction_pin;

};

#endif


