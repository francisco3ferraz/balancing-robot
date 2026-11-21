package main

import (
	"errors"
	"fmt"
	"strconv"
	"strings"
)

var ErrInvalidLine = errors.New("invalid line")

// ax,ay,az,temp,gx,gy,gz,accel_angle,gyro_rate,gyro_angle,angle,dt_us
type Sample struct {
	AccelX     int16   `json:"ax"` // raw counts
	AccelY     int16   `json:"ay"`
	AccelZ     int16   `json:"az"`
	Temp       int16   `json:"temp"` // raw; °C = Temp/340 + 36.53
	GyroX      int16   `json:"gx"`   // raw counts
	GyroY      int16   `json:"gy"`
	GyroZ      int16   `json:"gz"`
	AccelAngle float32 `json:"accel_angle"` // degrees
	GyroRate   float32 `json:"gyro_rate"`   // degrees per second
	GyroAngle  float32 `json:"gyro_angle"`  // degrees
	Angle      float32 `json:"angle"`       // degrees, complementary filter
	DtMicros   uint32  `json:"dt_us"`       // microseconds since the previous sample
}

func main() {
}

const numFields = 12

// parseLine parses one telemetry line. Every error wraps ErrInvalidLine.
func parseLine(line string) (Sample, error) {
	fields := strings.Split(line, ",")
	if len(fields) != numFields {
		return Sample{}, fmt.Errorf("%w: got %d fields, want %d", ErrInvalidLine, len(fields), numFields)
	}

	// Go evaluates the calls in a composite literal left to right,
	// so the fields are consumed in CSV column order.
	p := fieldParser{fields: fields}
	s := Sample{
		AccelX:     p.int16(),
		AccelY:     p.int16(),
		AccelZ:     p.int16(),
		Temp:       p.int16(),
		GyroX:      p.int16(),
		GyroY:      p.int16(),
		GyroZ:      p.int16(),
		AccelAngle: p.float32(),
		GyroRate:   p.float32(),
		GyroAngle:  p.float32(),
		Angle:      p.float32(),
		DtMicros:   p.uint32(),
	}
	if p.err != nil {
		return Sample{}, p.err
	}
	return s, nil
}

// fieldParser reads fields in order and remembers the first error
type fieldParser struct {
	fields []string
	i      int // index of the next field to read
	err    error
}

// next returns the next field, or ok=false if an earlier field failed.
func (p *fieldParser) next() (field string, ok bool) {
	if p.err != nil {
		return "", false
	}
	field = p.fields[p.i]
	p.i++
	return field, true
}

// fail records err for the field just read, wrapped with ErrInvalidLine.
func (p *fieldParser) fail(err error) {
	p.err = fmt.Errorf("%w: field %d: %w", ErrInvalidLine, p.i-1, err)
}

func (p *fieldParser) int16() int16 {
	f, ok := p.next()
	if !ok {
		return 0
	}
	v, err := strconv.ParseInt(f, 10, 16)
	if err != nil {
		p.fail(err)
	}
	return int16(v)
}

func (p *fieldParser) float32() float32 {
	f, ok := p.next()
	if !ok {
		return 0
	}
	v, err := strconv.ParseFloat(f, 32)
	if err != nil {
		p.fail(err)
	}
	return float32(v)
}

func (p *fieldParser) uint32() uint32 {
	f, ok := p.next()
	if !ok {
		return 0
	}
	v, err := strconv.ParseUint(f, 10, 32)
	if err != nil {
		p.fail(err)
	}
	return uint32(v)
}
