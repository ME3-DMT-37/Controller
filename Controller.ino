#include <Audio.h>
#include <EEPROM.h>
#include <SD.h>
#include <SerialFlash.h>
#include <SPI.h>
#include <Wire.h>

#include "Tuner.h"

// ----------------------------------------------------------------

AudioInputAnalog          adc;
AudioAnalyzeNoteFrequency note;
AudioAnalyzePeak          peak;
AudioConnection           patchCord(adc, note);
AudioConnection           patchCord2(adc, peak);

Tuner tuner(3, 4, 5, 6, 10, 9, 11);

// ----------------------------------------------------------------

int led_pin = 13;

int silent = 0;

int string = 1;

// ----------------------------------------------------------------

void setup() {

  pinMode(led_pin, OUTPUT);

  // open serial port
  Serial.begin(9600);

  // delay for serial port initialisation
  delay(1000);

  // recover forward speeds from memory
  for (int i = 0; i < 6; i++) {

    tuner.speed_forward[i] = EEPROM.read(i);
    tuner.speed_reverse[i] = EEPROM.read(i + 6);

    Serial.printf("string %d: +%d / -%d\n", i + 1, tuner.speed_forward[i], tuner.speed_reverse[i]);

  }

  // allocate memory to audio library
  AudioMemory(30);
  note.begin(0.15);

  tuner.detuned[string] = true;
  tuner.calibrated[string] = true;
  tuner.tuned[string] = false;

}

// ----------------------------------------------------------------

void loop() {

  if (sample()) {

    if (!tuner.detuned[string]) {
      tuner.detune(string, REVERSE);
    } else if (!tuner.calibrated[string]) {
      tuner.calibrate(string, FORWARD);
    } else if (!tuner.tuned[string]) {
      tuner.tune(string);
    }

    silent = 0;

  } else {
    
    tuner.motorStop(string);

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
    tuner.note = note.read();
    tuner.peak = peak.read() * 1.2;

    // remove mains interference and check peak voltage then raise flag
    if ((tuner.note > 55) && (tuner.peak > 0.3)) {
      return true;
    } else {
      return false;
    }

  } else {
    return false;
  }

}

