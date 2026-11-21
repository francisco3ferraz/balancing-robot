# Firmware

## Files

| File | What it does |
| --- | --- |
| `src/main.cpp` | Starts everything, runs the loop, prints the data |
| `src/mpu6050.cpp` | Talks to the sensor |
| `src/tilt.cpp` | Turns sensor readings into a tilt angle |

## Startup

1. Check the sensor is there (it must answer with ID 0x68).
2. Configure it. Each setting is read back to confirm it was saved.
3. If anything fails, stop and print the error once a second.

Sensor settings:

| Setting | Value | Why |
| --- | --- | --- |
| Clock | Gyro-based | More stable than the internal one |
| Low-pass filter | 44 Hz | Removes vibration the 100 Hz loop can't measure correctly |
| Accelerometer range | ±2 g | Most precise; the calibration assumes it |
| Gyroscope range | ±500 °/s | ±250 °/s was too small for quick movements |

## Loop

Runs exactly 100 times a second. Each time it:
1. Reads the sensor
2. Updates the tilt angle
3. Prints one line of data (see [telemetry.md](telemetry.md))

## Tilt angle

There are two ways to measure tilt, and each has a weakness:

| Sensor | Good | Bad |
| --- | --- | --- |
| Accelerometer | Doesn't drift | Noisy, and fooled by bumps and vibration |
| Gyroscope | Smooth | Drifts away over time |

A **complementary filter** combines them: it follows the gyroscope for quick changes, and the accelerometer slowly corrects the drift.

```
angle = 0.98 × (angle + gyro_rate × dt) + 0.02 × accel_angle
```

## Things to know

- The sensor keeps its settings when the ESP32 resets. Unplug USB to reset the sensor too.
- Use `float`, not `double`: the ESP32 only has fast hardware for `float`.
- Serial runs at 921600 baud. The garbage printed at boot is normal.
