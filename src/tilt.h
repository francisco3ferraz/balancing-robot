#pragma once

#include "mpu6050.h"

// Tilt around the X axis, in degrees. 0 = Y pointing up; forward tilt is negative.
struct tilt {
    float accel_angle;
    float gyro_rate;   // degrees per second
    float gyro_angle;  // gyro only, drifts; kept for telemetry
    float angle;       // complementary filter
};

// dt is the time since the previous call, in seconds.
struct tilt tilt_update(const struct mpu_raw *raw, float dt);
