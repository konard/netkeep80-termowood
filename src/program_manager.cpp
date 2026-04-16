#include <Arduino.h>
#include "program_manager.h"
#include "temperature.h"
#include <EEPROM.h>
#include <cstring>

namespace {
bool programRunning = false;
unsigned long programStartTime = 0;
unsigned long stageStartTime = 0;
int currentStage = 0;
unsigned int remainingTime = 0;
Program currentProgram;
}

void initProgramManager() {
  EEPROM.begin(sizeof(Program) * 10);
}

bool loadProgram(int programNumber, Program& program) {
  if (programNumber < 0 || programNumber >= 10) {
    return false;
  }

  EEPROM.get(sizeof(Program) * programNumber, program);
  if (program.numStages == 0 || program.numStages > MAX_STAGES) {
    return false;
  }

  return true;
}

bool saveProgram(int programNumber, const Program& program) {
  if (programNumber < 0 || programNumber >= 10) {
    return false;
  }

  EEPROM.put(sizeof(Program) * programNumber, program);
  EEPROM.commit();
  return true;
}

void createNewProgram(Program& program) {
  std::strcpy(program.name, "New program");
  program.numStages = 0;

  for (int i = 0; i < MAX_STAGES; ++i) {
    program.stages[i].duration = 0;
    program.stages[i].temperature = 0.0f;
  }
}

void startProgram(const Program& program) {
  currentProgram = program;
  programRunning = true;
  programStartTime = millis();
  currentStage = 0;
  stageStartTime = programStartTime;
  remainingTime = currentProgram.stages[0].duration * 60;
  setTargetTemperature(currentProgram.stages[0].temperature);
}

void stopProgram() {
  programRunning = false;
  disableHeating();
}

bool isProgramRunning() {
  return programRunning;
}

int getCurrentStage() {
  return currentStage;
}

Program getCurrentProgram() {
  return currentProgram;
}

unsigned int getRemainingTime() {
  return remainingTime;
}

void updateProgram() {
  if (!programRunning) {
    return;
  }

  const unsigned long currentMillis = millis();
  const unsigned long stageDuration = currentProgram.stages[currentStage].duration * 60UL;
  const unsigned long elapsedTime = (currentMillis - stageStartTime) / 1000UL;

  if (elapsedTime >= stageDuration) {
    ++currentStage;
    if (currentStage >= static_cast<int>(currentProgram.numStages)) {
      stopProgram();
      return;
    }

    stageStartTime = currentMillis;
    remainingTime = currentProgram.stages[currentStage].duration * 60;
    setTargetTemperature(currentProgram.stages[currentStage].temperature);
    return;
  }

  remainingTime = stageDuration - elapsedTime;
}
