#include "person_sensor.h"
#include <Wire.h>

// Fetch the latest results from the sensor. Returns false if the read didn't
// succeed.
bool person_sensor_read(person_sensor_results_t* results) {
    Wire.requestFrom(PERSON_SENSOR_I2C_ADDRESS, sizeof(person_sensor_results_t));
    if (Wire.available() != sizeof(person_sensor_results_t)) {
    //if (Wire.available() != 32) { 
        Serial.print("error in sensor read. bytes avail : ");
        Serial.println(Wire.available());
        Serial.print("bytes expected: ");
        Serial.println(sizeof(person_sensor_results_t));
        Serial.println();
        return false;
    }

    int8_t* results_bytes = (int8_t*)(results);
    for (unsigned int i=0; i < sizeof(person_sensor_results_t); ++i) {
    //for (unsigned int i=0; i < 32; ++i) {
        results_bytes[i] = Wire.read();
    }
    return true;
}

// Writes the value to the sensor register over the I2C bus.
void person_sensor_write_reg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(PERSON_SENSOR_I2C_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}