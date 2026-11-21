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

// Y offset = (16020 + (−16590)) / 2 ≈ −285
// Y scale = (16020 − (−16590)) / 2 ≈ 16305
#define AY_OFFSET  -285

// Z offset = (−20110 + 13000) / 2 ≈ −3555
// Z Scale = (13000 − (−20110)) / 2 ≈ 16555
#define AZ_OFFSET -3555

bool read_regs(uint8_t reg, uint8_t *buf, uint8_t len);
bool read_reg(uint8_t reg, uint8_t *out);
bool write_reg(uint8_t reg, uint8_t value);
int16_t merge_two_bytes(uint8_t high, uint8_t low);
float compute_accel_angle(int16_t ay, int16_t az);

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
        Serial.printf("PWR_MGMT_1 = 0x%02X\n", pwr);  // expect 0x01, not 0x40
    } else {
        Serial.println("read_reg failed");
    }
}

void loop() {
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

    Serial.printf("%d,%d,%d,%d,%d,%d,%d,%.2f\n",
                  accel_xout, accel_yout, accel_zout, temp_out,
                  gyro_xout, gyro_yout, gyro_zout, accel_angle);
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