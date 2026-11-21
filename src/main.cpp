#include <Arduino.h>
#include <Wire.h>

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

// Y offset = (16020 + (−16590)) / 2 ≈ −285
// Y scale = (16020 − (−16590)) / 2 ≈ 16305
#define AY_OFFSET  -285

// Z offset = (−20110 + 13000) / 2 ≈ −3555
// Z Scale = (13000 − (−20110)) / 2 ≈ 16555
#define AZ_OFFSET -3555

#define GYRO_FS_500DPS   0x08   // GYRO_CONFIG: FS_SEL=1 (bits 4:3) -> +/-500 deg/s
#define GYRO_LSB_PER_DPS 65.5f  // sensitivity at +/-500 deg/s

// Complementary filter: weight on the gyro prediction each pass.
// Time constant ~ dt * ALPHA / (1 - ALPHA) = ~0.5 s at 100 Hz.
#define FILTER_ALPHA 0.98f

bool read_regs(uint8_t reg, uint8_t *buf, uint8_t len);
bool read_reg(uint8_t reg, uint8_t *out);
bool write_reg(uint8_t reg, uint8_t value);
int16_t merge_two_bytes(uint8_t high, uint8_t low);
float compute_accel_angle(int16_t ay, int16_t az);

#define LOOP_PERIOD_US 10000  // 100 Hz

uint32_t iteration_due_at;   // micros() value when the next pass may start
uint32_t last_iteration_at;  // micros() value when the previous pass started

void setup() {
    Serial.begin(921600);
    delay(1000);
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

    last_iteration_at = micros();
    iteration_due_at = last_iteration_at + LOOP_PERIOD_US;
}

void loop() {
    while ((int32_t)(micros() - iteration_due_at) < 0) {
    }
    iteration_due_at += LOOP_PERIOD_US;

    uint32_t now = micros();
    uint32_t dt_us = now - last_iteration_at;  // real time since the previous pass
    last_iteration_at = now;

    uint8_t buf[14]; // registers 0x3B..0x48, each value H then L
    if (!read_regs(REG_ACCEL_XOUT_H, buf, 14)) {
        Serial.println("read_regs failed");
        return;
    }

    int16_t accel_xout = merge_two_bytes(buf[0], buf[1]);
    int16_t accel_yout = merge_two_bytes(buf[2], buf[3]);
    int16_t accel_zout = merge_two_bytes(buf[4], buf[5]);
    int16_t temp_out = merge_two_bytes(buf[6], buf[7]);
    int16_t gyro_xout = merge_two_bytes(buf[8], buf[9]);
    int16_t gyro_yout = merge_two_bytes(buf[10], buf[11]);
    int16_t gyro_zout = merge_two_bytes(buf[12], buf[13]);

    float accel_angle = compute_accel_angle(accel_yout, accel_zout);
    float gyro_rate = -(float)gyro_xout / GYRO_LSB_PER_DPS; // should be inverted or not depended on how the chip is mounted
    float dt = dt_us / 1000000.0f; // to seconds

    static float gyro_angle = 0.0f;  // initialised once, keeps its value between passes
    gyro_angle += gyro_rate * dt;    // updated every pass

    static float angle = 0.0f;
    static bool first_pass = true;
    if (first_pass) {
        // Start from the accelerometer so we don't converge from 0 for ~2 s.
        angle = accel_angle;
        first_pass = false;
    } else {
        angle = FILTER_ALPHA * (angle + gyro_rate * dt) + (1.0f - FILTER_ALPHA) * accel_angle;
    }

    // CSV: ax,ay,az,temp,gx,gy,gz,accel_angle,gyro_rate,gyro_angle,angle,dt_us
    Serial.printf("%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%lu\n",
                  accel_xout, accel_yout, accel_zout, temp_out,
                  gyro_xout, gyro_yout, gyro_zout, accel_angle,
                  gyro_rate, gyro_angle, angle, (unsigned long)dt_us);
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
    return read_regs(reg, out, 1);
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

// Tilt around X in degrees, from offset-corrected Y and Z accel.
// 0 = Y pointing up; forward tilt is negative.
float compute_accel_angle(int16_t ay, int16_t az) {
    float y = ay - AY_OFFSET;
    float z = az - AZ_OFFSET;
    return atan2f(z, y) * (180.0f / (float)M_PI);
}

int16_t merge_two_bytes(uint8_t high, uint8_t low) {
    return (int16_t)((high << 8) | low);
}
