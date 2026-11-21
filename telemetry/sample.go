package main

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
)

var ErrInvalidLine = errors.New("invalid line")

// Sample is one telemetry line from the firmware. The fields are in CSV column order.
type Sample struct {
	AccelX     int16   `json:"ax"`
	AccelY     int16   `json:"ay"`
	AccelZ     int16   `json:"az"`
	Temp       int16   `json:"temp"` // °C = Temp/340 + 36.53
	GyroX      int16   `json:"gx"`
	GyroY      int16   `json:"gy"`
	GyroZ      int16   `json:"gz"`
	AccelAngle float32 `json:"accel_angle"` // degrees
	GyroRate   float32 `json:"gyro_rate"`   // degrees per second
	GyroAngle  float32 `json:"gyro_angle"`  // degrees
	Angle      float32 `json:"angle"`       // degrees, complementary filter
	DtMicros   uint32  `json:"dt_us"`
}

const csvHeader = "ax,ay,az,temp,gx,gy,gz,accel_angle,gyro_rate,gyro_angle,angle,dt_us"

func parseLine(line string) (Sample, error) {
	fields := strings.Split(line, ",")
	if len(fields) != 12 {
		return Sample{}, fmt.Errorf("%w: got %d fields, want 12", ErrInvalidLine, len(fields))
	}

	var ints [7]int16
	for i := range ints {
		v, err := strconv.ParseInt(fields[i], 10, 16)
		if err != nil {
			return Sample{}, badField(i, err)
		}
		ints[i] = int16(v)
	}

	var floats [4]float32
	for i := range floats {
		v, err := strconv.ParseFloat(fields[7+i], 32)
		if err != nil {
			return Sample{}, badField(7+i, err)
		}
		floats[i] = float32(v)
	}

	dt, err := strconv.ParseUint(fields[11], 10, 32)
	if err != nil {
		return Sample{}, badField(11, err)
	}

	return Sample{
		AccelX:     ints[0],
		AccelY:     ints[1],
		AccelZ:     ints[2],
		Temp:       ints[3],
		GyroX:      ints[4],
		GyroY:      ints[5],
		GyroZ:      ints[6],
		AccelAngle: floats[0],
		GyroRate:   floats[1],
		GyroAngle:  floats[2],
		Angle:      floats[3],
		DtMicros:   uint32(dt),
	}, nil
}

func badField(i int, err error) error {
	return fmt.Errorf("%w: field %d: %w", ErrInvalidLine, i, err)
}
