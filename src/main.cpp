#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define MPU_ADDR 0x68

// MPU-6050 registers
#define REG_ACCEL_XOUT_H 0x3B
#define REG_ACCEL_XOUT_L 0x3C
#define REG_ACCEL_YOUT_H 0x3D
#define REG_ACCEL_YOUT_L 0x3E
#define REG_ACCEL_ZOUT_H 0x3F
#define REG_ACCEL_ZOUT_L 0x40
#define REG_TEMP_OUT_H   0x41
#define REG_TEMP_OUT_L   0x42
#define REG_GYRO_XOUT_H  0x43
#define REG_GYRO_XOUT_L  0x44
#define REG_GYRO_YOUT_H  0x45
#define REG_GYRO_YOUT_L  0x46
#define REG_GYRO_ZOUT_H  0x47
#define REG_GYRO_ZOUT_L  0x48
#define REG_PWR_MGMT_1   0x6B
#define REG_WHO_AM_I     0x75

bool read_regs(uint8_t reg, uint8_t *buf, uint8_t len);
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
    /*
    uint8_t who;
    if (read_reg(REG_WHO_AM_I, &who) {
        Serial.printf("WHO_AM_I = 0x%02X\n", who);
    } else {
        Serial.println("read_reg failed");
    }
    */
    uint8_t buf[2];

    if (read_regs(REG_ACCEL_XOUT_H, buf, 2)) {
        Serial.printf("ACCEL_X  H=0x%02X  L=0x%02X\n", buf[0], buf[1]);
    } else {
        Serial.println("read_regs failed");
    }

    delay(1000);
}

bool read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    // fails if the sensor NACKed or sent too few bytes.
    if (Wire.requestFrom(MPU_ADDR, len) != len) {
        return false;
    }
    for (size_t i = 0; i < len; i++)
    {
        buf[i] = Wire.read();
    }

    return true;
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