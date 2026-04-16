#include <cstring>

#include <unity.h>

#include "EEPROM.h"
#include "program_manager.h"

namespace {
unsigned long currentMillis = 0;
float lastTargetTemperature = 0.0f;
int targetTemperatureCalls = 0;
int disableHeatingCalls = 0;

Program makeProgram(unsigned int stages) {
  Program program;
  std::strcpy(program.name, "Test program");
  program.numStages = stages;

  for (unsigned int i = 0; i < MAX_STAGES; ++i) {
    program.stages[i].duration = i < stages ? 1 : 0;
    program.stages[i].temperature = 100.0f + static_cast<float>(i * 10);
  }

  return program;
}

void setMockMillis(unsigned long milliseconds) {
  currentMillis = milliseconds;
}

void resetRuntimeMocks() {
  setMockMillis(0);
  lastTargetTemperature = 0.0f;
  targetTemperatureCalls = 0;
  disableHeatingCalls = 0;
}
}

EEPROMClass EEPROM;

unsigned long millis() {
  return currentMillis;
}

void delay(unsigned long milliseconds) {
  currentMillis += milliseconds;
}

void setTargetTemperature(float target) {
  lastTargetTemperature = target;
  ++targetTemperatureCalls;
}

void disableHeating() {
  ++disableHeatingCalls;
}

void setUp() {
  stopProgram();
  initProgramManager();
  resetRuntimeMocks();
}

void tearDown() {
  stopProgram();
}

void test_create_new_program_initializes_defaults() {
  Program program = makeProgram(MAX_STAGES);

  createNewProgram(program);

  TEST_ASSERT_EQUAL_STRING("New program", program.name);
  TEST_ASSERT_EQUAL_UINT(0, program.numStages);

  for (int i = 0; i < MAX_STAGES; ++i) {
    TEST_ASSERT_EQUAL_UINT(0, program.stages[i].duration);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, program.stages[i].temperature);
  }
}

void test_save_and_load_program_roundtrip() {
  Program saved = makeProgram(2);
  Program loaded = {};

  TEST_ASSERT_TRUE(saveProgram(1, saved));
  TEST_ASSERT_TRUE(EEPROM.wasCommitted());
  TEST_ASSERT_TRUE(loadProgram(1, loaded));

  TEST_ASSERT_EQUAL_STRING(saved.name, loaded.name);
  TEST_ASSERT_EQUAL_UINT(saved.numStages, loaded.numStages);
  TEST_ASSERT_EQUAL_UINT(saved.stages[0].duration, loaded.stages[0].duration);
  TEST_ASSERT_EQUAL_FLOAT(saved.stages[0].temperature, loaded.stages[0].temperature);
  TEST_ASSERT_EQUAL_UINT(saved.stages[1].duration, loaded.stages[1].duration);
  TEST_ASSERT_EQUAL_FLOAT(saved.stages[1].temperature, loaded.stages[1].temperature);
}

void test_program_stage_uses_full_duration_before_advancing() {
  Program program = makeProgram(2);

  startProgram(program);
  TEST_ASSERT_TRUE(isProgramRunning());
  TEST_ASSERT_EQUAL_INT(0, getCurrentStage());
  TEST_ASSERT_EQUAL_UINT(60, getRemainingTime());
  TEST_ASSERT_EQUAL_FLOAT(100.0f, lastTargetTemperature);
  TEST_ASSERT_EQUAL_INT(1, targetTemperatureCalls);

  setMockMillis(31000);
  updateProgram();
  TEST_ASSERT_TRUE(isProgramRunning());
  TEST_ASSERT_EQUAL_INT(0, getCurrentStage());
  TEST_ASSERT_EQUAL_UINT(29, getRemainingTime());

  setMockMillis(59000);
  updateProgram();
  TEST_ASSERT_TRUE(isProgramRunning());
  TEST_ASSERT_EQUAL_INT(0, getCurrentStage());
  TEST_ASSERT_EQUAL_UINT(1, getRemainingTime());

  setMockMillis(60000);
  updateProgram();
  TEST_ASSERT_TRUE(isProgramRunning());
  TEST_ASSERT_EQUAL_INT(1, getCurrentStage());
  TEST_ASSERT_EQUAL_UINT(60, getRemainingTime());
  TEST_ASSERT_EQUAL_FLOAT(110.0f, lastTargetTemperature);
  TEST_ASSERT_EQUAL_INT(2, targetTemperatureCalls);
}

void test_program_stops_after_final_stage_duration() {
  Program program = makeProgram(1);

  startProgram(program);

  setMockMillis(59000);
  updateProgram();
  TEST_ASSERT_TRUE(isProgramRunning());
  TEST_ASSERT_EQUAL_INT(0, getCurrentStage());

  setMockMillis(60000);
  updateProgram();
  TEST_ASSERT_FALSE(isProgramRunning());
  TEST_ASSERT_EQUAL_INT(1, disableHeatingCalls);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_create_new_program_initializes_defaults);
  RUN_TEST(test_save_and_load_program_roundtrip);
  RUN_TEST(test_program_stage_uses_full_duration_before_advancing);
  RUN_TEST(test_program_stops_after_final_stage_duration);
  return UNITY_END();
}
