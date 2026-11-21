# Telemetry tool

Reads the data from the ESP32, saves it, and shows it as a live chart in the browser.

## Use

From the `telemetry/` folder:

| Command | What it does |
| --- | --- |
| `go run . -port /dev/ttyUSB0` | Live from the board, saved to `logs/` |
| `go run . -replay file.csv` | Plays a saved file, no board needed |

Open http://localhost:8080/. Stop with Ctrl+C.

Options: `-log file.csv` picks the log file, `-addr` changes the web address.

## Chart

- Shows the last 10 seconds: accelerometer angle, gyroscope angle and filtered angle.
- **Pause** (or space bar) freezes it, so you can hover over values and drag to zoom.

## Data format

Each line from the board:
```
ax,ay,az,temp,gx,gy,gz,accel_angle,gyro_rate,gyro_angle,angle,dt_us
```

| Columns | Meaning |
| --- | --- |
| `ax` … `gz` | Raw sensor readings |
| `accel_angle`, `gyro_angle`, `angle` | Tilt in degrees: accelerometer, gyroscope, filtered |
| `gyro_rate` | Rotation speed in °/s |
| `dt_us` | Time since the previous line, in µs (about 10000) |

## Notes

- Only one program can use the serial port. Stop this tool before uploading firmware.
- After editing `index.html`, restart the tool; reloading the browser isn't enough.
- Tests: `go test ./...`
