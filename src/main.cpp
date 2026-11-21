#include <Arduino.h>

#include "mpu6050.h"
#include "tilt.h"

#define LOOP_PERIOD_US 10000  // 100 Hz

uint32_t iteration_due_at;   // micros() value when the next pass may start
uint32_t last_iteration_at;  // micros() value when the previous pass started

void setup() {
    Serial.begin(921600);
    delay(1000);
    if (!mpu_init()) {
        while (true) {
            // repeat so a monitor opened late still sees why it stopped
            Serial.println("mpu_init failed, halted");
            delay(1000);
        }
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

    struct mpu_raw raw;
    if (!mpu_read(&raw)) {
        Serial.println("read_regs failed");
        return;
    }

    struct tilt t = tilt_update(&raw, dt_us / 1000000.0f);

    // CSV: ax,ay,az,temp,gx,gy,gz,accel_angle,gyro_rate,gyro_angle,angle,dt_us
    Serial.printf("%d,%d,%d,%d,%d,%d,%d,%.2f,%.2f,%.2f,%.2f,%lu\n",
                  raw.ax, raw.ay, raw.az, raw.temp,
                  raw.gx, raw.gy, raw.gz, t.accel_angle,
                  t.gyro_rate, t.gyro_angle, t.angle, (unsigned long)dt_us);
}
