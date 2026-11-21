# Self-balancing robot

A two-wheeled self-balancing robot built on an ESP32 and an MPU-6050 sensor.
The firmware is written from scratch, without sensor libraries. That includes the I2C
register driver, the tilt estimate and the control loop. A small Go tool streams the
sensor data from the serial port to a live chart in the browser.

## What works so far

- I2C driver for the MPU-6050: identity check, configuration with read-back
  verification, and 14-byte burst reads.
- The firmware refuses to start if the sensor is missing or misconfigured.
- Tilt angle from the accelerometer (calibrated), from the gyroscope (integrated), and
  from a complementary filter that combines the two.
- A fixed-rate 100 Hz loop.
- Telemetry over serial as CSV, with a live browser chart, logging and replay.

The motors, the frame and the balance controller are not built yet.

## Repository layout

```
src/            ESP32 firmware (PlatformIO, Arduino framework)
  main.cpp        setup, the 100 Hz loop, the CSV output
  mpu6050.*       I2C driver for the sensor
  tilt.*          calibration and the complementary filter
telemetry/      Go tool: serial reader, CSV log, live chart
docs/           how everything works, and what was measured
```

## Quick start

Firmware (needs [PlatformIO](https://platformio.org/)):

```
pio run                  # build
pio run -t upload        # flash the ESP32
pio device monitor       # raw serial output at 921600 baud
```

Live chart (needs Go):

```
cd telemetry
go run . -port /dev/ttyUSB0          # live from the board
go run . -replay testdata/filter_test.csv   # replay a recording, no board needed
```

Then open http://localhost:8080/.

Only one program can use the serial port at a time. Close the monitor or the telemetry
tool before uploading.

## Documentation

- [Hardware](docs/hardware.md): parts, wiring, and how the sensor is mounted
- [Firmware](docs/firmware.md): the sensor driver, the tilt filter and the loop
- [Telemetry](docs/telemetry.md): the Go tool and the CSV format
- [Measurements](docs/measurements.md): calibration, noise and filter results
