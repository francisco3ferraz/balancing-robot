#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define MPU_ADDR 0x68
#define REG_WHO_AM_I 0x75

bool read_reg(uint8_t reg, uint8_t *out);

void setup() {
    Serial.begin(115200);
    delay(1000);
    Wire.begin(SDA_PIN, SCL_PIN);
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
    if (Wire.endTransmission(false) != 0) {  // address or register byte NACKed
        return false;
    }
    if (Wire.requestFrom(MPU_ADDR, 1) != 1) {  // sensor sent fewer bytes than asked
        return false;
    }
    *out = Wire.read();
    return true;
}
