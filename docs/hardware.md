# Hardware

## Parts

| Part | Model |
| --- | --- |
| Microcontroller | Keyestudio KS5019 (ESP32) |
| Motion sensor | GY-521 (MPU-6050): accelerometer + gyroscope |
| Motors | 2 × yellow "TT" gear motors (3–6 V type) |
| Motor driver | Keyestudio KS0063 (L298N) |
| Battery | 2 × 18650 cells in series (7.4 V, 8.4 V full), holder with switch |
| 5 V supply | JOY-IT SBC-Buck01 (LM2596 step-down converter) |

## Sensor wiring

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

## Motor driver setup

| Jumper | Setting | Why |
| --- | --- | --- |
| UR1–UR4 | **Removed** | They pull IN1–IN4 up to 5 V, which the ESP32 can't take |
| CSA, CSB | On | Connect the motor current to ground; motors don't run without them |
| 5V_EN | On | The board makes its own 5 V from the battery |

- With 5V_EN on, the driver's **5V terminal is an output**. Never connect another 5 V supply to it.
- ENA and ENB are always pulled up to 5 V on the board.
- The driver drops about 2 V, so the motors get about 5.3–6.4 V.

## Power

- The converter is set to **5.0 V** (measured with a multimeter; its own display is about 0.15 V off).
- Check the converter's output with a meter before connecting anything to it. Turning the trimmer counter-clockwise lowers the voltage.
- Disconnect the battery before changing any wiring.

## Board notes

- The pin labels are on the back of the KS5019 only.
- There is no LED you can control; the red LED only shows power.
- The driver's solder joints are bare on the back: mount it on spacers, never flat on metal.
