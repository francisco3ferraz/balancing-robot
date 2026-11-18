#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define MPU_ADDR 0x68
#define REG_WHO_AM_I 0x75
#define REG_PWR_MGMT_1 0x6B

bool read_reg(uint8_t reg, uint8_t *out);
bool write_reg(uint8_t reg, uint8_t value);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Wire.begin(SDA_PIN, SCL_PIN);
    if (!write_reg(REG_PWR_MGMT_1, 0x01)) {
        Serial.println("write_reg failed");
    }
    delay(100);

    uint8_t pwr;
    if (read_reg(REG_PWR_MGMT_1, &pwr)) {
        Serial.printf("PWR_MGMT_1 = 0x%02X\n", pwr);  // expect 0x01, not 0x40
    } else {
        Serial.println("read_reg failed");
    }
}

void loop() {
    uint8_t who;
    if (read_reg(REG_WHO_AM_I, &who)) {
        Serial.printf("WHO_AM_I = 0x%02X\n", who);
    } else {
        Serial.println("read_reg failed");
    }
    delay(1000);
}

bool read_reg(uint8_t reg, uint8_t *out) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    // fails if the sensor NACKed or sent too few bytes.
    if (Wire.requestFrom(MPU_ADDR, 1) != 1) {
        return false;
    }
    *out = Wire.read();
    return true;
}

bool write_reg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    if (Wire.endTransmission(true) != 0) {  // address or register byte NACKed
        return false;
    }
    return true;
}