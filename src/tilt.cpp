#include <math.h>

#include "tilt.h"

// Y offset = (16020 + (−16590)) / 2 ≈ −285
// Y scale = (16020 − (−16590)) / 2 ≈ 16305
#define AY_OFFSET  -285

// Z offset = (−20110 + 13000) / 2 ≈ −3555
// Z Scale = (13000 − (−20110)) / 2 ≈ 16555
#define AZ_OFFSET -3555

// Complementary filter: weight on the gyro prediction each pass.
// Time constant ~ dt * ALPHA / (1 - ALPHA) = ~0.5 s at 100 Hz.
#define FILTER_ALPHA 0.98f

static float gyro_angle = 0.0f;
static float angle = 0.0f;
static bool first_pass = true;

static float compute_accel_angle(int16_t ay, int16_t az) {
    float y = ay - AY_OFFSET;
    float z = az - AZ_OFFSET;
    return atan2f(z, y) * (180.0f / (float)M_PI);
}

struct tilt tilt_update(const struct mpu_raw *raw, float dt) {
    float accel_angle = compute_accel_angle(raw->ay, raw->az);
    float gyro_rate = -(float)raw->gx / MPU_GYRO_LSB_PER_DPS; // should be inverted or not depended on how the chip is mounted

    gyro_angle += gyro_rate * dt;

    if (first_pass) {
        // Start from the accelerometer so we don't converge from 0 for ~2 s.
        angle = accel_angle;
        first_pass = false;
    } else {
        angle = FILTER_ALPHA * (angle + gyro_rate * dt) + (1.0f - FILTER_ALPHA) * accel_angle;
    }

    return (struct tilt){accel_angle, gyro_rate, gyro_angle, angle};
}
