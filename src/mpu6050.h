#pragma once

#include <stdint.h>

// Gyro sensitivity at the ±500 °/s range set by mpu_init().
#define MPU_GYRO_LSB_PER_DPS 65.5f

// One burst read of the MPU-6050, raw counts as the chip reports them.
struct mpu_raw {
    int16_t ax, ay, az;
    int16_t temp;
    int16_t gx, gy, gz;
};

bool mpu_init(void);
bool mpu_read(struct mpu_raw *out);
