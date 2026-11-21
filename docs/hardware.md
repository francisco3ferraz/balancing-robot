# Hardware

## Parts

| Part | Model |
| --- | --- |
| Microcontroller | Keyestudio KS5019 (ESP32) |
| Motion sensor | GY-521 (MPU-6050): accelerometer + gyroscope |

## Wiring

| Sensor pin | ESP32 pin |
| --- | --- |
| VCC | 3V3 |
| GND | GND |
| SCL | IO22 |
| SDA | IO21 |

The other sensor pins stay unconnected.
Use 3.3 V, not 5 V: the ESP32 pins can't take 5 V.

## Sensor orientation

- The sensor's Y axis points up.
- The robot tilts around the X axis.
- 0° = upright. Tilting forward gives a negative angle.

If the sensor is mounted differently, the axes in `src/tilt.cpp` must change.

## Board notes

- The pin labels are on the back of the KS5019 only.
- There is no LED you can control; the red LED only shows power.
