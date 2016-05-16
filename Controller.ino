int motor_pin[] = {3, 4, 5, 6, 9, 10};
int direction_pin = 11;

void setup() {

  pinMode(motor_pin[0], OUTPUT);
  pinMode(motor_pin[1], OUTPUT);
  pinMode(motor_pin[2], OUTPUT);
  pinMode(motor_pin[3], OUTPUT);
  pinMode(motor_pin[4], OUTPUT);
  pinMode(motor_pin[5], OUTPUT);
  pinMode(direction_pin, OUTPUT);

}

void loop() {

  for (int i = 0; i < 6; i++) {
    motorRun(i, 100);
    delay(1000);
  }

}

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
    digitalWrite(motor_pin[i], direction);
  }

  // set only requested motor on
  analogWrite(motor_pin[motor], duty);

  // set direction pin
  digitalWrite(direction_pin, direction);

}

