#include <Arduino.h>
#include <Wire.h>

#include "mpu6050.h"

#define SDA_PIN 21
#define SCL_PIN 22

#define MPU_ADDR 0x68

// MPU-6050 registers
#define REG_GYRO_CONFIG  0x1B
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

#define GYRO_FS_500DPS 0x08  // GYRO_CONFIG: FS_SEL=1 (bits 4:3) -> +/-500 deg/s

static bool read_regs(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    // fails if the sensor NACKed or sent too few bytes.
    if (Wire.requestFrom(MPU_ADDR, len) != len) {
        return false;
    }
    for (size_t i = 0; i < len; i++) {
        buf[i] = Wire.read();
    }
    return true;
}

static bool read_reg(uint8_t reg, uint8_t *out) {
    return read_regs(reg, out, 1);
}

static bool write_reg(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(value);
    if (Wire.endTransmission(true) != 0) {  // address or register byte NACKed
        return false;
    }
    return true;
}

static int16_t merge_two_bytes(uint8_t high, uint8_t low) {
    return (int16_t)((high << 8) | low);
}

void mpu_init(void) {
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(400000);  // MPU-6050 supports 400 kHz fast mode

    if (!write_reg(REG_PWR_MGMT_1, 0x01)) {
        Serial.println("write_reg failed");
    }
    delay(100);

    uint8_t pwr;
    if (read_reg(REG_PWR_MGMT_1, &pwr)) {
        Serial.printf("PWR_MGMT_1 = 0x%02X\n", pwr);  // expect 0x01
    } else {
        Serial.println("read_reg failed");
    }

    if (!write_reg(REG_GYRO_CONFIG, GYRO_FS_500DPS)) {
        Serial.println("write_reg failed");
    }

    uint8_t gyro_config;
    if (read_reg(REG_GYRO_CONFIG, &gyro_config)) {
        Serial.printf("GYRO_CONFIG = 0x%02X\n", gyro_config);  // expect 0x08
    } else {
        Serial.println("read_reg failed");
    }
}

bool mpu_read(struct mpu_raw *out) {
    uint8_t buf[14];  // registers 0x3B..0x48, each value H then L
    if (!read_regs(REG_ACCEL_XOUT_H, buf, 14)) {
        return false;
    }
    out->ax   = merge_two_bytes(buf[0], buf[1]);
    out->ay   = merge_two_bytes(buf[2], buf[3]);
    out->az   = merge_two_bytes(buf[4], buf[5]);
    out->temp = merge_two_bytes(buf[6], buf[7]);
    out->gx   = merge_two_bytes(buf[8], buf[9]);
    out->gy   = merge_two_bytes(buf[10], buf[11]);
    out->gz   = merge_two_bytes(buf[12], buf[13]);
    return true;
}
