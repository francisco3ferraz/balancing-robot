# Measurements

All measured on the real sensor, sitting still unless noted.

## Calibration

The accelerometer reads slightly off. These offsets are subtracted from the raw values:

| Axis | Offset (raw) |
| --- | --- |
| Y | −285 |
| Z | −3555 |

## Gyroscope drift

- At rest it reads about 0.5 °/s instead of 0.
- On its own, its angle drifted 30° in one minute. This is why the filter is needed.

## Filter results

| Test | Result |
| --- | --- |
| At rest | About 10× less noise than the accelerometer alone |
| 94 s still | No drift (the gyroscope alone drifted 45°) |
| Strong shaking | Wrong during the shake, correct again within about 1 s |

## Low-pass filter (44 Hz)

| Noise at rest | Off | On |
| --- | --- | --- |
| Accelerometer angle | 0.30° | 0.13° |
| Gyroscope | 0.083 °/s | 0.040 °/s |

About half the noise, and the angle itself didn't change.
